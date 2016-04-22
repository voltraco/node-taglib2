# SYNOPSIS
taglib version 2 bindings

# USAGE
If you are trying to *read* tags "in the hot path", meaning you care about
performance, don't use a native module, stay in javascript-land.

[`Here`](https://github.com/leetreveil/musicmetadata) is a good module for
parsing meta data. However, if you need to *write* data, you may find this
library useful. Keep in mind, performance is acceptable for one-off writes,
but it takes about 1 second for 100 writes on a low-end macbook.

### WRITING TAGS
A `cover`, can be any image type, but you need to specify the mimetype,
to find the mimetype, we use [`node-mime`](https://github.com/broofa/node-mime).

```js
const taglib = require('taglib2')
const mime = require('node-mime')

const props = {
  artist: 'Howlin\' Wolf',
  title: 'Evil is goin\' on',
  album: 'Smokestack Lightnin\'',
  comment: 'Chess Master Series',
  genre: 'blues',
  year: 1951,
  track: 3,
  mimetype: mime('./cover.jpg'),
  cover: fs.readFileSync('./cover.jpg')
}

taglib.writeTagsSync('./file.mp3', props)
```

### READING TAGS

```js
const taglib = require('taglib2')
const tags = taglib.readTagsSync('./file.mp3')
```

#### OUTPUT
`tags.cover` will be an array of buffers that contain image data.

```json
{
  "artist": "Howlin' Wolf",
  "albumartist": "Howlin' Wolf",
  "title": "Evil is goin' on",
  "album": "Smokestack Lightnin'",
  "comment": "Chess Master Series",
  "composer": "Chester Burnett",
  "genre": "blues",
  "year": 1951,
  "track": 3,
  "cover": [],
  "bitrate": 192,
  "bpm": 120,
  "samplerate": 44100,
  "channels": 2,
  "time": "1:30",
  "length": 90
}
```

