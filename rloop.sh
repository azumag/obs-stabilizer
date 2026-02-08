#!/bin/bash

# STATE.md の1行目を読み込んで状態に応じて処理

while true; do
	# STATE.md の1行目を読み込む（ファイルがない場合はIDLE）
	state=$(head -n 1 STATE.md 2>/dev/null || echo "IDLE")

	# 改行文字を削除
	state=$(echo "$state" | tr -d '\n\r')

	echo "[$(date '+%Y-%m-%d %H:%M:%S')] 状態: '$state'"

	case "$state" in
	"IDLE")
		opencode run "/flow-resolve" --agent='zai'
		;;
	"PLANNED")
		opencode run "/implement" --agent='zai'
		;;
	"CHANGE_REQUESTED")
		opencode run "/implement" --agent='zai'
		;;
	"IMPLEMENTED")
		opencode run "/flow-review" --agent='zai'
		;;
	"REVIEW_PASSED")
		opencode run "/flow-qa" --agent='zai'
		;;
	"QA_PASSED")
		opencode run "/sweep" --agent='zai'
		;;
	*)
		echo "[$(date '+%Y-%m-%d %H:%M:%S')] 状態: 不明 - '$state'"
		opencode run "/flow-resolve" --agent='zai'
		;;
	esac

	sleep 10
done
