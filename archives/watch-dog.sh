#!/bin/bash

sleep 10

WATCH_DIR="docs"
ZELLIJ_CMD='zellij action write-chars "/clear" && zellij action write 13 && zellij action write-chars "/f-start-co" && zellij action write 32 && zellij action write-chars "bash gh を使って repository issue を取得し、実装するべき内容を考えて設計し、実装エージェントに依頼してください. issueが既に解決済みの場合は、ghでissueを閉じてください. issue がない場合は自分でコードの問題点を発見し、issueを発行し、設計し、実装エージェントに依頼してください. また前回の実装でpushしたコードがCIで落ちていないかどうか確認し、修正指示を実装エージェントに依頼してください. README.md を適切にアップデートすること。" && sleep 3 && zellij action write 13'

mkdir -p "$WATCH_DIR"

last_change=$(date +%s)

check_github_issues() {
	issue_count=$(gh issue list --state open --json id --jq '. | length' 2>/dev/null)
	echo "$issue_count"
}

while true; do
	current_time=$(date +%s)
	last_file_change=$(find "$WATCH_DIR" -type f -exec stat -f %m {} \; 2>/dev/null | sort -rn | head -1)

	if [ -z "$last_file_change" ]; then
		last_file_change=0
	fi

	if (($(echo "$current_time - $last_file_change" | bc) >= 300)); then
		eval "$ZELLIJ_CMD"
		last_change=$current_time
	fi

	issue_count=$(check_github_issues)
	if [ "$issue_count" -gt 0 ]; then
		eval "$ZELLIJ_CMD"
	fi

	sleep 86400
done
