## building the string that is embedded in "SendHTML"

```
cd web-ui
npm install
./node_modules/.bin/webpack
```

this creates a "dist" directory with a file `index.html`.

To get the string:

```
cat dist/index.html | tr '\n' ' ' | sed -r 's/"/\\"/g'
```
