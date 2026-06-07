use std::env;

use ws_rs::client::Client;

#[tokio::main]
async fn main() -> ws_rs::Result<()> {
    let url = env::args()
        .nth(1)
        .unwrap_or_else(|| "ws://127.0.0.1:8080/".to_string());
    let msg = env::args().nth(2).unwrap_or_else(|| "hello".to_string());

    let mut client = Client::new();
    client.connect(&url).await?;
    client.send_text(&msg).await?;
    if let Some(reply) = client.recv_text().await? {
        println!("{reply}");
    }
    client.close().await?;
    Ok(())
}
