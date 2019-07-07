## building the string that is embedded in "SendHTML"

You'll need 'xclip', 'node'.

```
cd web-ui
npm install
./build.sh
```

You'll have the contents of `dist/c_embed_string.txt` on your clipboard. Take this and paste it in WebServerAP.ino as the return value of `SendHTML`.

