# 100M Ethernet MAC for Xilinx FPGA

## 概要

中古Zynq FPGAボード `EBAZ4205` に搭載されている 100M Ethernet PHYを使って、PL上でEthernet通信を行うためのコア及びVivadoデザインです。

このデザインは株式会社フィックスターズ Tech Blog の記事(https://proc-cpuinfo.fixstars.com/2020/05/alveo-u50-10g-ethernet/) で説明されている10G MAC IPを参考に作られています。
オリジナルのリポジトリは https://github.com/fixstars/xg_mac です。

## 動作環境

以下のボードで動作確認を行っています。

* EBAZ4205

合成環境は以下のとおりです。Vivadoのバージョンは同じでないとプロジェクトの復元に失敗します。

* Ubuntu 20.04
* Vivado 2020.2
* PetaLinux 2020.2

## ライセンス

ほとんどオリジナルの10G Ethrenet MACのコードは残っていませんが、一部プロジェクト復元周りのスクリプトやFIFOのRTLを使っています。
10G Ethernet MAC IP自体は3条項BSDライセンスで公開されていますので、本プロジェクトのデザインも3条項BSDライセンスとします。

詳しくは本リポジトリに含まれている `LICENSE` ファイルを確認してください。

## 使用方法

Vivado 2020.2とPetaLinuxを使える状態にしておきます。
以下に、 Vivado 2020.2が `/tools/Xilinx/Vivado/2020.2` PetaLinuxが `~/petalinux` にインストールされている場合のコマンドを示します。

```
source /tools/Xilinx/Vivado/2020.2/settings64.sh
source ~/petalinux/settings.sh
```

この状態で、 `vivado/ebaz_server` 以下でmakeを実行してデザインを合成します。

```
cd vivado/ebaz_server
make
```

成功すれば `vivado/ebaz_server/design_1_wrapper.xsa` が生成されます。
次に、PetaLinuxのイメージをビルドします。このとき、イメージ書き込み先のSDカードをマウントし、そのマウント先を `SDCARD` 環境変数に設定しておきます。
例としてSDカードが `/media/sdcard` にマウントされている場合は以下のようになります。

```
export SDCARD=/media/sdcard
cd vivado/ebaz_server/petalinux/ebaz
make
```

途中、`petalinux-config` の設定画面が表示されるので、 `Exit` を選択して閉じます。

成功すると、SDカードにイメージがコピーされているので、アンマウントして `EBAZ4205` に挿して起動します。

## 動作確認

EBAZ4205を `192.168.4.0/24` のネットワークに接続します。開発マシンと一対一でつなぐのが簡単です。

開発マシンから `192.168.4.2` へpingを実行すると応答します。

```
$ ping 192.168.4.2
PING 192.168.4.2 (192.168.4.2) 56(84) bytes of data.
64 bytes from 192.168.4.2: icmp_seq=1 ttl=64 time=0.391 ms
64 bytes from 192.168.4.2: icmp_seq=2 ttl=64 time=0.195 ms
^C
--- 192.168.4.2 ping statistics ---
2 packets transmitted, 2 received, 0% packet loss, time 1019ms
rtt min/avg/max/mdev = 0.195/0.293/0.391/0.098 ms
```


また、シリアル経由でEBAZ4205へログインし、PS側のアドレスを `192.168.4.3` に設定します。

* ユーザー名: petalinux
* パスワード: petalinux

でログインできます。rootのパスワードは `root` です。


```
$ su
# ifconfig eth0 192.168.4.3 netmask 255.255.255.0
```

この状態で `192.168.4.3` にpingを実行するときちんと応答します。sshでの接続も可能です。

```
$ ping 192.168.4.3
PING 192.168.4.3 (192.168.4.3) 56(84) bytes of data.
64 bytes from 192.168.4.3: icmp_seq=1 ttl=64 time=0.577 ms
64 bytes from 192.168.4.3: icmp_seq=2 ttl=64 time=0.316 ms
64 bytes from 192.168.4.3: icmp_seq=3 ttl=64 time=0.332 ms
^C
--- 192.168.4.3 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2056ms
rtt min/avg/max/mdev = 0.316/0.408/0.577/0.119 ms

$ ssh -l petalinux 192.168.4.3
The authenticity of host '192.168.4.3 (192.168.4.3)' can't be established.
RSA key fingerprint is SHA256:vc4h7U44yE9ZwKINzqaDcllAutXgXycc5WsKu8hf2nA.
Are you sure you want to continue connecting (yes/no/[fingerprint])? yes
Warning: Permanently added '192.168.4.3' (RSA) to the list of known hosts.
petalinux@192.168.4.3's password: 
ebaz:~$ 
```

