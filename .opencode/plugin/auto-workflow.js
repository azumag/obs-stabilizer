const processedSessions = new Set()
export const AutoWorkflowPlugin = async ({ client }) => {
  console.log('AutoWorkflowPlugin loaded')
  return {
    event: async ({ event }) => {
      try {
        if (event.type === "session.created") {
          const session = event.properties?.info
          const sessionId = session?.id
          console.log('Session created, ID:', sessionId)
          if (sessionId) {
            processedSessions.add(sessionId)
            console.log('Added to processedSessions:', Array.from(processedSessions))
          }
        }
        if (event.type === "message.updated") {
          const message = event.properties?.info
          console.log('Message updated:', message?.role, 'completed:', !!message?.time?.completed, 'sessionID:', message?.sessionID, 'hasError:', !!message?.error)
          if (message?.role === "assistant" && message?.time?.completed && !message?.error) {
            const sessionId = message?.sessionID
            console.log('Checking sessionID in processedSessions:', sessionId, processedSessions.has(sessionId))
            if (sessionId && processedSessions.has(sessionId)) {
              processedSessions.delete(sessionId)
              console.log('Submitting automatic prompt')
              await client.tui.appendPrompt({
                text: "結合テストを実行し、実装に全く問題がないことを確認せよ。もし失敗したら、修正せよ。修正がなくなったら、githubのリポジトリからissueを取得し、それを解決する実装をせよ。issueがゼロになるまで繰り返せ"
              })
              await client.tui.executeCommand({
                command: "prompt.submit"
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
