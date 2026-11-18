# Latent Voice: Fire sample for Unreal Engine

An Unreal Engine 5.8 project for the Mac and Windows (Windows is a preview): say ファイア and a campfire lights, say it again and it flares up, say ストップ and it goes out.

This repository has the sample only. The plugin comes from the developer portal. Add it before you open the project.

## What you need

- A Mac with Apple silicon, or a Windows PC (x64; a preview). Intel Macs are not supported.
- Unreal Engine 5.8 and the Latent Voice Unreal plugin 0.1.2 or later.
- On a Mac, Xcode; on Windows, Visual Studio with the C++ tools. Each in a version that Unreal Engine 5.8 supports.
- This is a C++ project: the plugin comes as source and is built with it.
- A folder path with ASCII letters only. Spaces are fine.

## Try it

1. Clone this repository.
2. Sign in to the developer portal, download the Unreal plugin, and create an AccessKey for **サンプルプロジェクト(焚き火)**. The sample's words belong to that project.
3. Unpack the plugin so that `Plugins/LatentVoice/LatentVoice.uplugin` exists in this folder.
4. Open `FireUnreal.uproject`. When it asks to rebuild the missing modules, choose **Yes**.
5. Open **Tools > Latent Voice > SDK のセットアップ**, paste the key and press **セットアップ**. The files go to `Content/LatentVoice/` and stay out of version control.
6. Press **Play**, allow the microphone, and say ファイア, then ストップ. On Windows, allow microphone access in Settings, under Privacy & security. The first start needs the network for the license check.

To package a game of your own, see the plugin's `Install.md`.

## The sample

The level is `Content/Fire/FireMap`. The Blueprints are in `Content/Fire/Blueprints`.

## License

This sample is under the [MIT License](LICENSE). The plugin isn't part of this repository and has its own license.

The MIT License doesn't cover the sound. `SourceAssets/FireLoop.wav` and `Content/Fire/FireLoop.uasset`, made from it, are
[Steady Burning Fireplace Crackling 31](https://sfxmint.com/sounds/ambience-fireplace-31) from SFXMint, under CC0 1.0, and it's AI-generated.

Unreal Engine is a trademark of Epic Games, Inc. This sample is not made or endorsed by Epic Games.

---

# Latent Voice: Fire サンプル（Unreal Engine）

「ファイア」と言うと焚き火がつき、もう一度言うと炎が大きく揺れ、「ストップ」と言うと消える、Mac と Windows 用の Unreal Engine 5.8 プロジェクトです（Windows はプレビュー）。

このリポジトリにはサンプルだけが入っています。プラグインは開発者ポータルから入手します。プロジェクトを開く前に追加してください。

## 必要なもの

- Apple シリコンの Mac、または Windows の PC（x64、プレビュー）。Intel Mac には対応していません。
- Unreal Engine 5.8 と Latent Voice Unreal プラグイン 0.1.2 以降。
- Mac では Xcode、Windows では Visual Studio（C++ のツール）。どちらも Unreal Engine 5.8 が対応するバージョン。
- C++ プロジェクトであること。プラグインはソースで提供され、プロジェクトと一緒にビルドされます。
- 英数字だけのフォルダーパス。スペースは使えます。

## 試す

1. このリポジトリをクローンします。
2. 開発者ポータルにログインし、Unreal 用プラグインをダウンロードして、**サンプルプロジェクト(焚き火)** の AccessKey を作ります。サンプルの単語はこのプロジェクトのものです。
3. プラグインを展開し、このフォルダーに `Plugins/LatentVoice/LatentVoice.uplugin` ができるように置きます。
4. `FireUnreal.uproject` を開きます。足りないモジュールを再ビルドするか聞かれたら、**Yes** を選びます。
5. **Tools > Latent Voice > SDK のセットアップ** を開き、キーを貼り付けて **セットアップ** を押します。ファイルは `Content/LatentVoice/` に入り、バージョン管理には含まれません。
6. **Play** を押し、マイクを許可して、ファイア、ストップ と話します。Windows では、設定の「プライバシーとセキュリティ」でマイクを許可します。初回は、ライセンスの確認にネットワークが必要です。

自分のゲームをパッケージ化するときは、プラグインの `Install.md` を見てください。

## サンプルの場所

レベルは `Content/Fire/FireMap`、Blueprint は `Content/Fire/Blueprints` にあります。

## ライセンス

このサンプルは [MIT ライセンス](LICENSE)（[参考訳](LICENSE.ja.md)）です。プラグインはこのリポジトリに含まれず、別のライセンスです。

音は MIT ライセンスの対象外です。`SourceAssets/FireLoop.wav` と、それから作った `Content/Fire/FireLoop.uasset` は SFXMint の
[Steady Burning Fireplace Crackling 31](https://sfxmint.com/sounds/ambience-fireplace-31) で、CC0 1.0 の AI 生成音です。

Unreal Engine は Epic Games, Inc. の商標です。このサンプルは Epic Games が作成・推奨したものではありません。
