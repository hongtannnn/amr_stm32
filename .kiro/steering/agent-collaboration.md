---
inclusion: manual
description: Guidelines for two-way task delegation between Kiro and Antigravity IDE via the .agents/ folder
---

# Agent Collaboration with Antigravity

This project uses a two-way file-based communication system between Kiro and Antigravity IDE located in the `.agents/` folder.

## When to Delegate to Antigravity

Delegate a task to Antigravity when:
- The task is independent and does not require Kiro's IDE context (e.g., research, analysis, brainstorming)
- The task is computationally heavy and can run in parallel while Kiro continues other work
- The user explicitly asks to use Antigravity for a task

Do NOT delegate when:
- The task requires direct file editing in the IDE
- The task depends on Kiro's current context or conversation history
- The task is simple enough to complete immediately

## Shared Context

Before writing a task, read relevant files from `.agents/context/` to avoid repeating project information in every task:
- `project.md` — tech stack and architecture
- `conventions.md` — coding style and patterns
- `current-sprint.md` — active goals and blockers
- `decisions.md` — key decisions already made

## Archiving to History

After a task cycle is complete (result has been read), archive the exchange to `.agents/history/` using the Task-ID as filename prefix:

```
.agents/history/<Task-ID>_<agent>_task.md
.agents/history/<Task-ID>_<agent>_result.md
```

## Delegating a Task to Antigravity

1. Check `.agents/antigravity/status.md` — do not assign if status is `processing`
2. Write the task to `.agents/antigravity/task.md` using this format:

```markdown
## Task: <title>
**File:** <relative path to relevant file, if any>
**Context:** <brief context Antigravity needs to understand the task>
**Question/Action:** <exactly what Antigravity should do>
**Task-ID:** <current timestamp, e.g. 2026-05-24T13:00:00>
```

3. Update `.agents/antigravity/status.md` → set status to `idle`
4. Run Antigravity CLI as a background process (non-blocking) using the `control_pwsh_process` tool with action "start". Use absolute paths in the prompt. The output will stream to the terminal for the user to monitor:

```
agy --print-timeout 2m --print "Read the file <absolute_path>\.agents\antigravity\AGENTS.md for instructions, then process the task in <absolute_path>\.agents\antigravity\task.md"
```

5. Immediately continue with other work. Do NOT wait or poll for the result.

## Reading Antigravity's Response

When the user asks to read Antigravity's result:
1. Check `.agents/antigravity/status.md` — if still `processing`, inform the user it's not ready yet
2. If `done`, read `.agents/antigravity/result.md` and incorporate the response into the conversation
3. Reset `.agents/antigravity/status.md` → set status to `idle`

## Processing Tasks from Antigravity

When triggered by a hook (`.agents/kiro/task.md` was edited):
1. Update `.agents/kiro/status.md` → set status to `processing`
2. Read `.agents/kiro/task.md`
3. Complete the task — this may involve reading files, editing code, or answering questions
4. Write the full response to `.agents/kiro/result.md` using this format:

```markdown
## Result: <task title>
**Status:** Completed / Partial / Failed
**Task-ID:** <same id from task>
**Response:**
<full response here>
```

5. Update `.agents/kiro/status.md` → set status to `done`
6. Inform the user that a task from Antigravity has been processed.
