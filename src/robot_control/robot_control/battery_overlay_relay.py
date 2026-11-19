import rclpy
from rclpy.node import Node
from sensor_msgs.msg import BatteryState
from rviz_2d_overlay_msgs.msg import OverlayText
from std_msgs.msg import ColorRGBA


class BatteryOverlayRelay(Node):
    def __init__(self):
        super().__init__('battery_overlay_relay')
        self.subscription = self.create_subscription(
            BatteryState,
            '/battery_state',
            self.battery_callback,
            10
        )
        self.publisher = self.create_publisher(
            OverlayText,
            '/battery_overlay_text',
            10
        )
        self.get_logger().info('Battery Overlay Relay Node started.')

    def battery_callback(self, msg: BatteryState):
        overlay = OverlayText()
        overlay.action = OverlayText.ADD
        overlay.width = 240
        overlay.height = 45
        overlay.horizontal_distance = 15
        overlay.vertical_distance = 15
        overlay.horizontal_alignment = OverlayText.RIGHT
        overlay.vertical_alignment = OverlayText.TOP
        overlay.text_size = 14.0
        overlay.line_width = 2
        overlay.font = "DejaVu Sans"

        # Background: dark translucent
        overlay.bg_color = ColorRGBA(r=0.0, g=0.0, b=0.0, a=0.6)

        pct = msg.percentage * 100.0 if msg.percentage <= 1.0 else msg.percentage
        volts = msg.voltage

        if pct < 20.0:
            overlay.fg_color = ColorRGBA(r=1.0, g=0.2, b=0.2, a=1.0)
            icon = "[!]"
        elif pct < 50.0:
            overlay.fg_color = ColorRGBA(r=1.0, g=0.8, b=0.0, a=1.0)
            icon = "[-]"
        else:
            overlay.fg_color = ColorRGBA(r=0.2, g=1.0, b=0.2, a=1.0)
            icon = "[OK]"

        overlay.text = f"{icon} BATTERY: {pct:.1f}% ({volts:.2f}V)"
        self.publisher.publish(overlay)


def main(args=None):
    rclpy.init(args=args)
    node = BatteryOverlayRelay()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
