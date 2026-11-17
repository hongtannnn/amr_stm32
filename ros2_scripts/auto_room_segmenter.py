#!/usr/bin/env python3
"""
auto_room_segmenter.py - Module tự động nhận diện và đánh số phòng từ bản đồ SLAM
Sử dụng cho hệ thống AMR chạy ROS 2 Nav2 trên Raspberry Pi 4B.

Thuật toán:
1. Đọc bản đồ tĩnh OccupancyGrid (qua topic /map hoặc từ file YAML/PGM).
2. Xử lý hình thái học (Morphological Erosion / Distance Transform) để cắt đứt các cửa hẹp.
3. Phân đoạn các vùng liên thông (Connected Components) thành từng phòng độc lập.
4. Trích xuất tâm phòng (Centroid) và tự động đánh số thứ tự từ 1 đến N.
5. Xuất cấu hình sang file rooms_config.yaml và ảnh trực quan hóa.
"""

import os
import sys
import yaml
import numpy as np

try:
    import cv2
except ImportError:
    print("[ERROR] Cần cài đặt OpenCV: pip3 install opencv-python")
    cv2 = None


class AutoRoomSegmenter:
    def __init__(self, min_room_area_m2=1.5, max_room_area_m2=20.0, door_width_m=0.8):
        """
        :param min_room_area_m2: Diện tích tối thiểu để coi là 1 phòng (m2), loại bỏ ngóc ngách nhỏ.
        :param max_room_area_m2: Diện tích tối đa để loại bỏ hành lang dài nếu hành lang không có cửa.
        :param door_width_m: Độ rộng ước lượng của cửa phòng (m) để chọn kích thước kernel xói mòn.
        """
        self.min_room_area_m2 = min_room_area_m2
        self.max_room_area_m2 = max_room_area_m2
        self.door_width_m = door_width_m

    def segment_from_file(self, yaml_path, output_dir=None):
        if cv2 is None:
            raise RuntimeError("OpenCV chưa được cài đặt.")

        if output_dir is None:
            output_dir = os.path.dirname(yaml_path)

        with open(yaml_path, 'r', encoding='utf-8') as f:
            map_meta = yaml.safe_load(f)

        img_rel_path = map_meta.get('image', 'map.pgm')
        img_full_path = os.path.join(os.path.dirname(yaml_path), img_rel_path)
        resolution = float(map_meta.get('resolution', 0.05))
        origin = map_meta.get('origin', [0.0, 0.0, 0.0])
        origin_x, origin_y = float(origin[0]), float(origin[1])

        # Đọc ảnh bản đồ PGM
        img = cv2.imread(img_full_path, cv2.IMREAD_GRAYSCALE)
        if img is None:
            raise FileNotFoundError(f"Không thể đọc file ảnh bản đồ: {img_full_path}")

        height, width = img.shape

        # Bước 1: Nhị phân hóa (Vùng tự do đi lại màu trắng, chướng ngại vật/chưa quét màu đen)
        # Trong map ROS: 254 (trắng) là free space, 0 (đen) là obstacle, 205 là unknown
        _, binary_free = cv2.threshold(img, 240, 255, cv2.THRESH_BINARY)

        # Bước 2: Tách cửa phòng bằng xói mòn (Erosion)
        door_pixels = max(3, int((self.door_width_m / resolution) / 2))
        kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (door_pixels, door_pixels))
        eroded = cv2.erode(binary_free, kernel, iterations=1)

        # Bước 3: Phân tích thành phần liên thông (Connected Components)
        num_labels, labels, stats, centroids = cv2.connectedComponentsWithStats(eroded, connectivity=8)

        pixel_area_m2 = resolution * resolution
        min_pixels = int(self.min_room_area_m2 / pixel_area_m2)
        max_pixels = int(self.max_room_area_m2 / pixel_area_m2)

        candidates = []
        for label_id in range(1, num_labels):
            area = stats[label_id, cv2.CC_STAT_AREA]
            if min_pixels <= area <= max_pixels:
                u_center, v_center = centroids[label_id]
                # Đổi tọa độ pixel sang tọa độ thực tế ROS (lưu ý ảnh có trục Y đảo ngược)
                real_x = origin_x + u_center * resolution
                real_y = origin_y + (height - 1 - v_center) * resolution
                candidates.append({
                    'label_id': label_id,
                    'area_m2': float(round(area * pixel_area_m2, 2)),
                    'pixel_center': (float(u_center), float(v_center)),
                    'real_x': float(round(real_x, 3)),
                    'real_y': float(round(real_y, 3))
                })

        # Bước 4: Sắp xếp thứ tự phòng chuẩn từ 1 đến N (từ dưới lên trên, từ trái sang phải)
        # Sắp theo Y tăng dần, sau đó theo X tăng dần
        candidates.sort(key=lambda item: (round(item['real_y'], 1), item['real_x']))

        # Gán số phòng từ 1 đến N
        room_dict = {}
        vis_img = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)

        colors = [
            (255, 100, 100), (100, 255, 100), (100, 100, 255),
            (255, 255, 100), (255, 100, 255), (100, 255, 255),
            (200, 150, 100), (150, 200, 100), (100, 150, 200)
        ]

        for idx, room in enumerate(candidates, start=1):
            room_name = f"Room_{idx}"
            room_dict[room_name] = {
                'room_number': idx,
                'x': room['real_x'],
                'y': room['real_y'],
                'yaw_deg': 0.0,
                'area_m2': room['area_m2']
            }

            # Vẽ màu phòng và đánh số lên ảnh trực quan
            mask = (labels == room['label_id']).astype(np.uint8)
            color = colors[(idx - 1) % len(colors)]
            vis_img[mask > 0] = color

            u_c, v_c = int(room['pixel_center'][0]), int(room['pixel_center'][1])
            cv2.circle(vis_img, (u_c, v_c), 5, (0, 0, 255), -1)
            cv2.putText(vis_img, f"Room {idx}", (u_c - 20, v_c - 10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 2, cv2.LINE_AA)

        # Xuất file cấu hình YAML
        out_yaml_path = os.path.join(output_dir, "rooms_config.yaml")
        with open(out_yaml_path, 'w', encoding='utf-8') as f:
            yaml.dump({'total_rooms': len(room_dict), 'rooms': room_dict},
                      f, allow_unicode=True, sort_keys=False)

        # Lưu ảnh phân đoạn
        vis_out_path = os.path.join(output_dir, "segmented_rooms_vis.png")
        cv2.imwrite(vis_out_path, vis_img)

        print(f"[OK] Đã nhận diện thành công {len(room_dict)} phòng!")
        print(f"[OK] Đã lưu tọa độ: {out_yaml_path}")
        print(f"[OK] Đã lưu ảnh trực quan: {vis_out_path}")
        return room_dict


if __name__ == '__main__':
    if len(sys.argv) > 1:
        map_yaml_file = sys.argv[1]
    else:
        map_yaml_file = "map.yaml"

    segmenter = AutoRoomSegmenter()
    if os.path.exists(map_yaml_file):
        segmenter.segment_from_file(map_yaml_file)
    else:
        print(f"[INFO] Hướng dẫn chạy: python3 auto_room_segmenter.py <duong_dan_file_map.yaml>")
