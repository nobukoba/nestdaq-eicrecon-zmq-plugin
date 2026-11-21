# SPADI と EICrecon の別コンテナ接続

SPADI の RARiS リプレイは TimeFrameBuilder の PUSH ソケットから TF を
送信します。このリポジトリのプラグインは EICrecon 側の PULL ソケットで
受信します。SPADI 側には `spadi-containers-second-trial` の RARiS レシピを
使用します。
検証に使用した SPADI 側の変更は [PR #8](https://github.com/nobukoba/spadi-containers-second-trial/pull/8) にあり、まだ `main` にマージされていません。

EIC の既存イメージを起動し、このリポジトリのソースをコンテナにマウントして
プラグインを自分でコンパイルします。ビルド結果はマウントした作業領域の
`build/nestdaq_zmq_source.so` に保存されます。

## 準備

ホストでリポジトリを取得します。

```sh
git clone https://github.com/nobukoba/nestdaq-eicrecon-zmq-plugin.git
cd nestdaq-eicrecon-zmq-plugin
```

EIC 環境は `eicweb/eic_xl:26.11.0-stable` で検証しました。EIC の設定を読み込む
ため、コンテナ内では `bash -l` を使ってください。別バージョンに変更した場合は
プラグインをビルドし直します。

## Docker

まず SPADI コンテナ `spadi-raris` を起動し、RARiS レシピの `run-start.sh` を
実行します。次にホストの別シェルから、EIC コンテナを起動します。

```sh
docker run --rm -it --network container:spadi-raris \
  -v "$PWD:/work" -w /work --entrypoint /bin/bash \
  eicweb/eic_xl:26.11.0-stable -l
```

コンテナ内でコンパイルし、受信を開始します。

```sh
./scripts/build.sh
export NESTDAQ_HOST=127.0.0.1 ZMQ_PORT=5501
export NESTDAQ_FEM_ID=0xc0a80a28
./scripts/run-eicrecon-apptainer.sh
```

`--network container:spadi-raris` は２つのコンテナでネットワーク名前空間を
共有します。SPADI が `127.0.0.1:5501` にバインドしていても接続できます。
Linux のホストネットワークを共有する構成でも接続可能です。別の Docker
ネットワーク名前空間では `127.0.0.1` は互いに異なり、SPADI のループバックに
バインドされたポートへは直接接続できません。

## Apptainer

EIC の SIF を用意し、ホストのリポジトリから起動します。

```sh
apptainer shell --bind "$PWD:/work" --pwd /work /path/to/eic_xl.sif
bash -l
cd /work
./scripts/build.sh
export NESTDAQ_HOST=127.0.0.1 ZMQ_PORT=5501
export NESTDAQ_FEM_ID=0xc0a80a28
./scripts/run-eicrecon-apptainer.sh
```

通常の Apptainer はホストネットワークを共有します。`--net` を使用する場合は
接続先を利用環境に合わせて設定してください。

## 再生と結果の確認

受信側を起動してから、SPADI の Web Control で INIT DEVICE、INIT TASK、RUN
を実行します。`NESTDAQ_FEM_ID` の既定値は従来の `0xc0a80a29` です。
RARiS run 20 のサンプルでは `0xc0a80a28` にヒットがあります。
現在の変換は選択した FEM のチャンネル 0–47 だけを既存の３つの
`TOFBarrelADCTDC` コレクションへ入れます。この名前と cellID は変換例であり、
RARiS の検出器配置や較正が検証されたことを意味しません。

待機 5 ms の run 20 では 2,815 TF、68,956 ヒットを受信し、各 TF の３つの
コレクションの件数がリプレイ記録と一致することを確認しました。
TF 出力は PUSH/PULL です。EICrecon と ARTEMIS の両方を同時に接続すると
TF は両者に複製されず分配されます。別々に再生して比較してください。
