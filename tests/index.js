'use strict'
const fs = require('fs')
const taglib2 = require('../index')
const test = require('tape')
const mkdirp = require('mkdirp')
const path = require('path')
const rimraf = require('rimraf')
const http = require('http')

const TMP_PATH = path.join(__dirname, '/tmp')
const FIXTURES_PATH = path.join(__dirname, '/fixtures')

;(function setup () {
  try {
    fs.statSync(TMP_PATH)
  } catch (_) {
    mkdirp.sync(TMP_PATH)
  }

  rimraf.sync(TMP_PATH + '/*')

  const flacfilesource = [
    'http://www.eclassical.com',
    '/custom/eclassical/files/BIS1447-002-flac_24.flac'
  ].join('')

  const flacfile = FIXTURES_PATH + '/classical.flac'

  fs.stat(flacfile, (err, stat) => {
    if (!err) return onReady()

    const ws = fs.createWriteStream(flacfile)
    http.get(flacfilesource, res => {
      res.pipe(ws)
      ws.on('end', onReady)
    })
  })
})()

function onReady () {
  test('extract images from flac', assert => {
    // extrat the images in the flac file (this file we know has 6 images)
    const p = FIXTURES_PATH + '/classical.flac'
    const ps = fs.statSync(p)
    assert.ok(ps.size > 0, 'flac file downloaded properly')

    const tags = taglib2.readTagsSync(p, { md5: true })
    assert.equal(tags.pictures.length, 6)

    // write the first one to the tmp directory
    const mime = tags.pictures[0].mimetype.split('/')
    const writepath = TMP_PATH + '/classical.' + mime[1]
    fs.writeFileSync(writepath, tags.pictures[0].picture)

    // compare the extracted one to the original image
    const extractedImage = fs.readFileSync(writepath)
    const originalImage = fs.readFileSync(FIXTURES_PATH + '/classical.jpeg')
    assert.equal(tags.md5, '5d6a560f13c2b9c7cbf99b3ac7dbbe81')
    assert.equal(extractedImage.length, originalImage.length)
    assert.end()
  })

  test('sync write/read', assert => {
    function getRandomYear () {
      const date = new Date()
      const from = date.setFullYear(1877, 0, 1)
      const to = (new Date()).getTime()
      return new Date(from + Math.random() * (to - from)).getFullYear()
    }

    const rn = Math.floor(Math.random() * 100)
    const rn_year = getRandomYear()

    const audiopath = FIXTURES_PATH + '/sample-output.mp3'
    fs.writeFileSync(audiopath, fs.readFileSync(FIXTURES_PATH + '/sample.mp3'))

    const imagepath = FIXTURES_PATH + '/sample.jpg'
    const imagefile = fs.readFileSync(imagepath)

    assert.ok(!!fs.statSync(audiopath))
    assert.ok(!!fs.statSync(imagepath))

    assert.throws(() => {
      taglib2.writeTagsSync()
    }, 'not enough arguments')

    const r = taglib2.writeTagsSync(audiopath, {
      artist: 'ärtist' + rn,
      albumartist: 'albumartist' + rn,
      title: 'title' + rn,
      album: 'album' + rn,
      comment: 'comment' + rn,
      genre: 'genre' + rn,
      year: rn_year,
      // track: 3 + rn,
      tracknumber: '3/' + rn,
      discnumber: '1/' + rn,
      composer: 'composer' + rn,
      bpm: parseInt(120, 10),
      // mimetype: 'image/jpeg',
      pictures: [{ mimetype: '', picture: imagefile }]
    })

    assert.ok(r)

    assert.throws(() => {
      taglib2.readTagsSync()
    }, 'not enough arguments')

    const tags = taglib2.readTagsSync(audiopath, { md5: true })

    assert.equal(tags.artist, 'ärtist' + rn)
    assert.equal(tags.albumartist, 'albumartist' + rn)
    assert.equal(tags.title, 'title' + rn)
    assert.equal(tags.bpm, 120)
    assert.equal(tags.album, 'album' + rn)
    assert.equal(tags.comment, 'comment' + rn)
    assert.equal(tags.genre, 'genre' + rn)
    assert.equal(tags.year, parseInt(rn_year, 10))
    assert.equal(tags.discnumber, '1/' + rn)
    assert.equal(tags.composer, 'composer' + rn)
    // assert.equal(tags.track, 3 + rn)
    assert.equal(tags.tracknumber, '3/' + rn)

    const tmpImagepath = TMP_PATH + '/sample.jpg'
    fs.writeFileSync(tmpImagepath, tags.pictures[0].picture)

    assert.equal(tags.bitrate, 192)
    assert.equal(tags.samplerate, 44100)
    assert.equal(tags.channels, 2)
    assert.equal(tags.length, 90)
    assert.equal(tags.time, '1:30')

    assert.equal(Buffer.compare(
      fs.readFileSync(tmpImagepath),
      fs.readFileSync(imagepath)
    ), 0)

    assert.end()
  })

  test('sync write/read m4a + jpg', assert => {
    const audiopath = FIXTURES_PATH + '/sample-output.m4a'
    fs.writeFileSync(audiopath, fs.readFileSync(FIXTURES_PATH + '/sample.m4a'))

    const imagepath = FIXTURES_PATH + '/sample.jpg'
    const imagefile = fs.readFileSync(imagepath)

    assert.comment('write a copy of an mp4 file with a new image')

    taglib2.writeTagsSync(audiopath, {
      pictures: [{
        mimetype: 'image/jpeg',
        picture: imagefile
      }]
    })

    assert.comment('read the tags from the new file')
    const tags = taglib2.readTagsSync(audiopath, { md5: true })

    assert.comment('write the picture to a tmp file')
    const tmpImagepath = TMP_PATH + '/sample.jpg'
    fs.writeFileSync(tmpImagepath, tags.pictures[0].picture)

    assert.comment('compare the image extracted to the image added')

    assert.equal(Buffer.compare(
      fs.readFileSync(tmpImagepath),
      fs.readFileSync(imagepath)
    ), 0)

    assert.end()
  })

  test('sync write/read m4a + png', assert => {
    const audiopath = FIXTURES_PATH + '/sample-output-png.m4a'
    fs.writeFileSync(audiopath, fs.readFileSync(FIXTURES_PATH + '/sample.m4a'))

    const imagepath = FIXTURES_PATH + '/sample.png'
    const imagefile = fs.readFileSync(imagepath)

    assert.comment('write a copy of an mp4 file with a new image')

    taglib2.writeTagsSync(audiopath, {
      pictures: [{
        mimetype: 'image/png',
        picture: imagefile
      }]
    })

    assert.comment('read the tags from the new file')
    const tags = taglib2.readTagsSync(audiopath, { md5: true })

    assert.comment('write the picture to a tmp file')
    const tmpImagepath = TMP_PATH + '/sample.png'
    fs.writeFileSync(tmpImagepath, tags.pictures[0].picture)

    assert.comment('compare the image extracted to the image added')

    assert.equal(Buffer.compare(
      fs.readFileSync(tmpImagepath),
      fs.readFileSync(imagepath)
    ), 0)

    assert.end()
  })
}

