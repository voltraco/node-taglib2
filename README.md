# SYNOPSIS
taglib version 2 bindings

# USAGE
Writing tags

```js
const taglib = require('taglib2')

const props = {
  artist: 'Howlin\' Wolf',
  title: 'Evil is goin\' on',
  album: 'Smokestack Lightnin\'',
  comment: 'Chess Master Series',
  genre: 'blues',
  year: 1951,
  track: 3,
  cover: fs.readFileSync('./cover.jpg')
}

taglib.writeTagsSync('./file.mp3', props)
```

Reading tags

```js
const taglib = require('taglib2')
let tags = taglib.readTagsSync('./file.mp3')
```

```json
{
  "artist": "Howlin' Wolf",
  "title": "Evil is goin' on",
  "album": "Smokestack Lightnin'",
  "comment": "Chess Master Series",
  "genre": "blues",
  "year": 1951,
  "track": 3,
  "cover": [<Buffer>]
}
```

