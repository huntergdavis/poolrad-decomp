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
Keep original game media and any derived copyrighted content under ignored
`resources/`. Do not put local network addresses, credentials, or private
device details into this public repository.
