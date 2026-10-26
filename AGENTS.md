# Agent instructions

Before debugging or re-implementing something, run `deja "<query>"` (or the
MCP recall tool) to check indexed prior agent sessions. Cite anything reused.

Prefix every assistant-authored user-visible commentary update and final
response with a fresh wall-clock timestamp in the form
`[YYYY-MM-DD HH:MM:SS TZ]` using `America/Los_Angeles` and its current
abbreviation. Immediately before each tool call, give a timestamped
commentary line naming the command or operation. Before each shell call,
compute the current Los Angeles time and put a literal first source line
`# [YYYY-MM-DD HH:MM:SS TZ] COMMAND: description` in the shell command.

Read [WORKSTREAMS.md](WORKSTREAMS.md) for branch ownership and PR gates.
For CTO progress updates, use the measured ASCII format in
[docs/BATTLE_CARD.md](docs/BATTLE_CARD.md).
Keep original game media and any derived copyrighted content under ignored
`resources/`. Do not put local network addresses, credentials, or private
device details into this public repository.

## Task system is the todo list (owner rule, all agents)

Every todo, assignment or follow-up lives in the Dunking Sheep task queue, so metrics and ownership survive across agent types. Do not keep a private todo list as the source of truth.

- Take work assigned to you with `dunkingsheep task next <your-pane-id>` (MCP `task_next`); the global default can hand you unrelated unowned work, which the leader owns, so do not claim it. Extend with `task heartbeat <id>`; record the evidence first with `task note <id> --reason "commit/PR, command run, what you did NOT verify"`, then finish with `task complete <id>`.
- Create work with `dunkingsheep task add "<title>" -p <prio>` and ALWAYS give it a body: the owner's ask, acceptance evidence, current state. A task with an empty body is not assignable work; fill it with `task update`.
- Every open task has an owner. Unowned means the leader owns it; never leave a task open and unclaimed without saying why.
- Read your tasks with `dunk-poll --tasks`; see queue health and per-actor claim/complete counts with `dunkingsheep task metrics`.
