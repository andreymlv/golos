use chrono::Utc;
use clap::Parser;
use tokio::{io::AsyncWriteExt, net::TcpListener};

#[derive(Parser, Debug)]
#[command(version, about = "Datetime server", long_about = None)]
struct Opt {
    #[arg(short, long, value_name = "PORT", default_value_t = 8080)]
    port: u16,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let opt = Opt::parse();
    let listener = TcpListener::bind(("127.0.0.1", opt.port)).await?;
    loop {
        match listener.accept().await {
            Ok((mut socket, addr)) => {
                println!("new client: {addr:?}");
                let now = Utc::now();
                socket.write_all(format!("{}\n", now).as_bytes()).await?;
            }
            Err(e) => println!("couldn't get client: {e:?}"),
        }
    }
}
