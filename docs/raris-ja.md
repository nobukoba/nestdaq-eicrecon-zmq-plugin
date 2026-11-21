# SPADI と EICrecon の別コンテナ接続

SPADI の RARiS リプレイは TimeFrameBuilder の PUSH ソケットから TF を
送信します。このプラグインは EICrecon 側の PULL ソケットで受信します。
SPADI 側は `spadi-containers-second-trial` の RARiS レシピを使用します。

Linux の Apptainer で EIC `26.11.0-stable` と SPADI を別々に起動し、
待機 5 ms の run 20 で 2,815 TF、68,956 ヒットを確認しました。各 TF の
３コレクションの件数もリプレイ記録と一致しました。

EICrecon と SPADI が Linux のホストネットワークを共有する場合、接続先は
`127.0.0.1:5501` です。Apptainer は通常ホストネットワークを共有します。
Docker の別コンテナでは通常 `127.0.0.1` が別々なので、ホストネットワーク、
または相互に到達できるアドレスとポートを設定してください。SPADI の出力が
`127.0.0.1` にバインドされている場合は、別ネットワークから到達できません。

EIC 環境内でこのリポジトリをビルドします。

```sh
./scripts/build.sh
export NESTDAQ_HOST=127.0.0.1 ZMQ_PORT=5501
export NESTDAQ_FEM_ID=0xc0a80a28
./scripts/run-eicrecon-apptainer.sh
```

受信側を起動してから、SPADI の Web Control で INIT DEVICE、INIT TASK、RUN
を実行します。`NESTDAQ_FEM_ID` の既定値は従来の `0xc0a80a29` です。
RARiS run 20 のサンプルでは `0xc0a80a28` にヒットがあります。
現在の変換は選択した FEM のチャンネル 0–47 だけを既存の３つの
`TOFBarrelADCTDC` コレクションへ入れます。この名前と cellID は変換例であり、
RARiS の検出器配置や較正が検証されたことを意味しません。

TF 出力は PUSH/PULL です。EICrecon と ARTEMIS の両方を同時に接続すると
TF は両者に複製されず分配されます。別々に再生して比較してください。
長いリプレイ待機時間を使う場合は、受信側の終了方法やタイムアウトにも注意してください。

Docker を利用する場合、イメージ作成時にプラグインをビルドできます。

```sh
docker build -t nestdaq-eicrecon:local .
docker run --rm --network host -e NESTDAQ_FEM_ID=0xc0a80a28 nestdaq-eicrecon:local
```

これは Linux のホストネットワーク利用例です。作成したイメージから SIF を
作り、`NESTDAQ_FEM_ID=0xc0a80a28 apptainer run image.sif` としても利用できます。
Docker イメージのビルドと SIF 変換は利用環境で行ってください。
