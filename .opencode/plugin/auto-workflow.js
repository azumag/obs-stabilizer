console.log('AutoWorkflowPlugin loading...')
export const AutoWorkflowPlugin = async ({ client, directory }) => {
  console.log('AutoWorkflowPlugin initialized', { directory })
  return {
    event: async ({ event }) => {
      try {
        console.log('Event received:', event.type)
        if (event.type === "message.updated") {
          const message = event.properties?.info
          console.log('Message:', message)
          if (message?.role === "assistant" && message?.time?.completed) {
            const sessionId = message?.sessionID
            console.log('Session ID:', sessionId)
            if (sessionId) {
              console.log('Sending prompt...')
              const appendResult = await client.tui.appendPrompt({
                body: {
                  text: "結合テストを実行し、実装に全く問題がないことを確認せよ。もし失敗したら、修正せよ。修正がなくなったら、githubのリポジトリからissueを取得し、それを解決する実装をせよ。issueがゼロになるまで繰り返せ"
                }
              })
              console.log('Append prompt result:', appendResult)
              
              const submitResult = await client.tui.submitPrompt({
                query: { directory }
              })
              console.log('Submit prompt result:', submitResult)
            }
          }
        }
      } catch (error) {
        console.error('Error in event handler:', error)
      }
    }
  }
}
