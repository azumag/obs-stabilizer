const processedSessions = new Set()
export const AutoWorkflowPlugin = async ({ client, directory }) => {
  return {
    event: async ({ event }) => {
      try {
        if (event.type === "session.created") {
          const session = event.properties?.info
          const sessionId = session?.id
          if (sessionId) {
            processedSessions.add(sessionId)
          }
        }
        if (event.type === "message.updated") {
          const message = event.properties?.info
          if (message?.role === "assistant" && message?.time?.completed) {
            const sessionId = message?.sessionID
            if (sessionId && processedSessions.has(sessionId)) {
              processedSessions.delete(sessionId)
              await client.tui.publish({
                body: {
                  type: "tui.prompt.append",
                  properties: {
                    text: "結合テストを実行し、実装に全く問題がないことを確認せよ。もし失敗したら、修正せよ。修正がなくなったら、githubのリポジトリからissueを取得し、それを解決する実装をせよ。issueがゼロになるまで繰り返せ"
                  }
                },
                query: { directory }
              })
              await client.tui.publish({
                body: {
                  type: "tui.command.execute",
                  properties: {
                    command: "prompt.submit"
                  }
                },
                query: { directory }
              })
            }
          }
        }
      } catch (error) {
        console.error('AutoWorkflowPlugin error:', error)
      }
    }
  }
}
