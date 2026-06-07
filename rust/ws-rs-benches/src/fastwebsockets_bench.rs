use std::future::Future;

use fastwebsockets::handshake;
use fastwebsockets::upgrade;
use fastwebsockets::{FragmentCollector, Frame, OpCode, WebSocket};
use http_body_util::Empty;
use hyper::body::{Bytes, Incoming};
use hyper::header::{CONNECTION, UPGRADE};
use hyper::server::conn::http1;
use hyper::service::service_fn;
use hyper::upgrade::Upgraded;
use hyper::{Request, Response};
use hyper_util::rt::TokioIo;
use tokio::net::{TcpListener, TcpStream};

pub struct SpawnExecutor;

impl<Fut> hyper::rt::Executor<Fut> for SpawnExecutor
where
    Fut: Future + Send + 'static,
    Fut::Output: Send + 'static,
{
    fn execute(&self, fut: Fut) {
        tokio::task::spawn(fut);
    }
}

async fn handle_echo(fut: upgrade::UpgradeFut) -> Result<(), fastwebsockets::WebSocketError> {
    let mut ws = FragmentCollector::new(fut.await?);
    loop {
        let frame = ws.read_frame().await?;
        match frame.opcode {
            OpCode::Close => break,
            OpCode::Text | OpCode::Binary => {
                ws.write_frame(frame).await?;
            }
            _ => {}
        }
    }
    Ok(())
}

async fn server_upgrade(
    mut req: Request<Incoming>,
) -> Result<Response<Empty<Bytes>>, fastwebsockets::WebSocketError> {
    let (response, fut) = upgrade::upgrade(&mut req)?;
    tokio::spawn(async move {
        let _ = handle_echo(fut).await;
    });
    Ok(response)
}

pub async fn run_echo_server(bind_host: &str, port: u16) {
    let addr = format!("{bind_host}:{port}");
    let listener = TcpListener::bind(&addr).await.expect("bind");
    println!("fastwebsockets echo server listening on {addr}");

    loop {
        let (stream, _) = match listener.accept().await {
            Ok(v) => v,
            Err(_) => break,
        };
        tokio::spawn(async move {
            let io = TokioIo::new(stream);
            let conn_fut = http1::Builder::new()
                .serve_connection(io, service_fn(server_upgrade))
                .with_upgrades();
            let _ = conn_fut.await;
        });
    }
}

pub async fn connect(
    host: &str,
    port: u16,
) -> Result<WebSocket<TokioIo<Upgraded>>, fastwebsockets::WebSocketError> {
    let stream = TcpStream::connect(format!("{host}:{port}")).await?;
    let host_hdr = format!("{host}:{port}");
    let req = Request::builder()
        .method("GET")
        .uri(format!("http://{host_hdr}/"))
        .header("Host", &host_hdr)
        .header(UPGRADE, "websocket")
        .header(CONNECTION, "upgrade")
        .header(
            "Sec-WebSocket-Key",
            fastwebsockets::handshake::generate_key(),
        )
        .header("Sec-WebSocket-Version", "13")
        .body(Empty::<Bytes>::new())
        .expect("valid request");

    let (mut ws, _) = handshake::client(&SpawnExecutor, req, stream).await?;
    ws.set_writev(false);
    ws.set_auto_pong(true);
    Ok(ws)
}

pub async fn send_text(
    ws: &mut WebSocket<TokioIo<Upgraded>>,
    text: &str,
) -> Result<(), fastwebsockets::WebSocketError> {
    ws.write_frame(Frame::text(text.as_bytes().into())).await
}

pub async fn recv_data(
    ws: &mut WebSocket<TokioIo<Upgraded>>,
) -> Result<Vec<u8>, fastwebsockets::WebSocketError> {
    loop {
        let frame = ws.read_frame().await?;
        match frame.opcode {
            OpCode::Close => return Ok(Vec::new()),
            OpCode::Text | OpCode::Binary => return Ok(frame.payload.to_vec()),
            _ => {}
        }
    }
}

pub async fn send_binary(
    ws: &mut WebSocket<TokioIo<Upgraded>>,
    data: &[u8],
) -> Result<(), fastwebsockets::WebSocketError> {
    ws.write_frame(Frame::binary(data.into())).await
}
