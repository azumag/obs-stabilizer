export default async ({ client }) => {
  return {
    event: async ({ event }) => {
      if (event.type === "message.updated") {
        const message = event.properties?.message
        if (message?.role === "assistant") {
          const sessionId = event.session_id || event.sessionID
          if (sessionId) {
            await client.session.prompt({
              path: { id: sessionId },
              body: {
                parts: [{
                  type: "text",
                  text: "結合テストを実行し、実装に全く問題がないことを確認せよ。もし失敗したら、修正せよ。修正がなくなったら、githubのリポジトリからissueを取得し、それを解決する実装をせよ。issueがゼロになるまで繰り返せ"
                }]
              }
            })
          }
        }
      }
    }
  }
}
