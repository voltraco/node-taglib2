#include <errno.h>
#include <string.h>

#include <v8.h>
#include <nan.h>
#include <node.h>
#include <node_buffer.h>

#include <fstream>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define NDEBUG
#define TAGLIB_STATIC
#include <taglib/tag.h>
#include <taglib/tlist.h>
#include <taglib/fileref.h>
#include <taglib/tfile.h>
#include <taglib/tfilestream.h>
#include <taglib/flacfile.h>
#include <taglib/mp4tag.h>
#include <taglib/mp4atom.h>
#include <taglib/mp4file.h>
#include <taglib/tpicturemap.h>
#include <taglib/tpropertymap.h>
#include <taglib/tbytevector.h>
#include <taglib/tbytevectorlist.h>

using namespace std;
using namespace v8;
using namespace node;

/*
 * Simple MD5 implementation
 *
 * Compile with: gcc -o md5 md5.c
 */
 
// Constants are the integer part of the sines of integers (in radians) * 2^32.
const uint32_t k[64] = {
  0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee ,
  0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501 ,
  0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be ,
  0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821 ,
  0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa ,
  0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8 ,
  0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed ,
  0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a ,
  0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c ,
  0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70 ,
  0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05 ,
  0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665 ,
  0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039 ,
  0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1 ,
  0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1 ,
  0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };
   
// r specifies the per-round shift amounts
const uint32_t r[] = {
  7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
  5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
  4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
  6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
 
// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
 
void to_bytes(uint32_t val, uint8_t *bytes) {
  bytes[0] = (uint8_t) val;
  bytes[1] = (uint8_t) (val >> 8);
  bytes[2] = (uint8_t) (val >> 16);
  bytes[3] = (uint8_t) (val >> 24);
}
 
uint32_t to_int32(const uint8_t *bytes) {
  return (uint32_t) bytes[0]
    | ((uint32_t) bytes[1] << 8)
    | ((uint32_t) bytes[2] << 16)
    | ((uint32_t) bytes[3] << 24);
}
 
void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest) {

  // These vars will contain the hash
  uint32_t h0, h1, h2, h3;

  // Message (to prepare)
  uint8_t *msg = NULL;

  size_t new_len, offset;
  uint32_t w[16];
  uint32_t a, b, c, d, i, f, g, temp;

  // Initialize variables - simple count in nibbles:
  h0 = 0x67452301;
  h1 = 0xefcdab89;
  h2 = 0x98badcfe;
  h3 = 0x10325476;

  //Pre-processing:
  //append "1" bit to message  
  //append "0" bits until message length in bits is 448 (mod 512)
  //append length mod (2^64) to message

  for (new_len = initial_len + 1; new_len % (512/8) != 448/8; ++new_len)
      ;

  // allocate a msg with new length
  msg = (uint8_t*)malloc(new_len + 8);
  // copy the original msg to the new one
  memcpy(msg, initial_msg, initial_len);
  // append "1" bit. Note that for a computer, 8bit is the minimum length of a datatype
  msg[initial_len] = 0x80; 
  for (offset = initial_len + 1; offset < new_len; ++offset)
      msg[offset] = 0; // append "0" bits

  // append the lower 32 bits of len in bits at the end of the buffer.
  to_bytes(initial_len*8, msg + new_len);
  // append the higher 32 bits of len in bits at the end of the buffer.
  to_bytes(initial_len >> 29, msg + new_len + 4);

  // Process the message in successive 512-bit chunks:
  //for each 512-bit chunk of message:
  for(offset=0; offset<new_len; offset += (512/8)) {

    // break chunk into sixteen 32-bit words w[j], from 0 to 15
    for (i = 0; i < 16; ++i)
      w[i] = to_int32(msg + offset + i*4);

    // Initialize hash value for this chunk:
    a = h0;
    b = h1;
    c = h2;
    d = h3;

    // Main loop:
    for(i = 0; i<64; i++) {

      if (i < 16) {
        f = (b & c) | ((~b) & d);
        g = i;
      } else if (i < 32) {
        f = (d & b) | ((~d) & c);
        g = (5*i + 1) % 16;
      } else if (i < 48) {
        f = b ^ c ^ d;
        g = (3*i + 5) % 16;          
      } else {
        f = c ^ (b | (~d));
        g = (7*i) % 16;
      }

      temp = d;
      d = c;
      c = b;
      b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
      a = temp;
    }

    // Add this chunk's hash to result so far:
    h0 += a;
    h1 += b;
    h2 += c;
    h3 += d;
  }

  // cleanup
  free(msg);

  //var char digest[16] := h0 append h1 append h2 append h3 //(Output is in little-endian)
  to_bytes(h0, digest);
  to_bytes(h1, digest + 4);
  to_bytes(h2, digest + 8);
  to_bytes(h3, digest + 12);
}


Local<Value> TagLibStringToString(TagLib::String s) {

  if (s.isEmpty()) return Nan::Null();

  TagLib::ByteVector str = s.data(TagLib::String::UTF16);

  return Nan::New<v8::String>(
    (uint16_t *) str.mid(2, str.size()-2).data(),
    s.size()
  ).ToLocalChecked();
}

TagLib::String StringToTagLibString(std::string s) {
  return TagLib::String(s, TagLib::String::UTF8);
}

bool isFile(const char *s) {
  struct stat st;
#ifdef _WIN32
  return ::stat(s, &st) == 0 && (st.st_mode & (S_IFREG));
#else
  return ::stat(s, &st) == 0 && (st.st_mode & (S_IFREG | S_IFLNK));
#endif
}

auto hasOption = [](
  Local<v8::Object> o,
  const std::string name) -> bool {

  return o->Has(Nan::New(name).ToLocalChecked());
};

auto getOptionString = [](
  Local<v8::Object> o,
  const std::string name) -> TagLib::String {

  auto r = o->Get(Nan::New(name).ToLocalChecked());
  std::string s = *v8::String::Utf8Value(r);
  return StringToTagLibString(s);
};

auto getOptionInt = [](
  Local<v8::Object> o,
  const std::string name) -> int {

  return o->Get(Nan::New(name).ToLocalChecked())->Int32Value();
};

NAN_METHOD(writeTagsSync) {
  Nan::HandleScope scope;
  Local<v8::Object> options;

  if (info.Length() < 2) {
    Nan::ThrowTypeError("Expected 2 arguments");
    return;
  }

  if (!info[0]->IsString()) {
    Nan::ThrowTypeError("Expected a path to audio file");
    return;
  }

  if (!info[1]->IsObject()) return;

  options = v8::Local<v8::Object>::Cast(info[1]);
  std::string audio_file = *v8::String::Utf8Value(info[0]->ToString());

  if (!isFile(audio_file.c_str())) {
    Nan::ThrowTypeError("Audio file not found");
    return;
  }

  TagLib::FileRef f(audio_file.c_str());
  TagLib::Tag *tag = f.tag();
  TagLib::PropertyMap map = f.properties();

  if (!tag) {
    Nan::ThrowTypeError("Could not parse file");
    return;
  }

  bool hasProps = false;

  if (hasOption(options, "albumartist")) {
    hasProps = true;
    TagLib::String value = getOptionString(options, "albumartist");
    map.erase(TagLib::String("ALBUMARTIST"));
    map.insert(TagLib::String("ALBUMARTIST"), value);
  }

  if (hasOption(options, "discnumber")) {
    hasProps = true;
    TagLib::String value = getOptionString(options, "discnumber");
    map.erase(TagLib::String("DISCNUMBER"));
    map.insert(TagLib::String("DISCNUMBER"), value);
  }

  if (hasOption(options, "tracknumber")) {
    hasProps = true;
    TagLib::String value = getOptionString(options, "tracknumber");
    map.erase(TagLib::String("TRACKNUMBER"));
    map.insert(TagLib::String("TRACKNUMBER"), value);
  }

  if (hasOption(options, "composer")) {
    hasProps = true;
    TagLib::String value = getOptionString(options, "composer");
    map.erase(TagLib::String("COMPOSER"));
    map.insert(TagLib::String("COMPOSER"), value);
  }

  if (hasProps) {
    f.setProperties(map);
  }

  if (hasOption(options, "artist")) {
    tag->setArtist(getOptionString(options, "artist"));
  }

  if (hasOption(options, "title")) {
    tag->setTitle(getOptionString(options, "title"));
  }

  if (hasOption(options, "album")) {
    tag->setAlbum(getOptionString(options, "album"));
  }

  if (hasOption(options, "comment")) {
    tag->setComment(getOptionString(options, "comment"));
  }

  if (hasOption(options, "genre")) {
    tag->setGenre(getOptionString(options, "genre"));
  }

  if (hasOption(options, "year")) {
    tag->setYear(getOptionInt(options, "year"));
  }

  if (hasOption(options, "track")) {
    tag->setTrack(getOptionInt(options, "track"));
  }

  if (hasOption(options, "pictures")) {
    auto pictures = options->Get(Nan::New("pictures").ToLocalChecked());
    Local<Array> pics = Local<Array>::Cast(pictures);
    unsigned int plen = pics->Length();

    TagLib::PictureMap picMap;
    bool hasPics = false;

    for (unsigned int i = 0; i < plen; i++) {
      Local<v8::Object> imgObj = Handle<Object>::Cast(pics->Get(i));

      if (!hasOption(imgObj, "mimetype")) {
        Nan::ThrowTypeError("mimetype required for each picture");
        return;
      }

      if (!hasOption(imgObj, "picture")) {
        Nan::ThrowTypeError("picture required for each item in pictures array");
        return;
      }

      auto mimetype = getOptionString(imgObj, "mimetype");
      auto picture = imgObj->Get(Nan::New("picture").ToLocalChecked());

      if (!picture.IsEmpty() && node::Buffer::HasInstance(picture->ToObject())) {

        char* buffer = node::Buffer::Data(picture->ToObject());
        const size_t blen = node::Buffer::Length(picture->ToObject());
        TagLib::ByteVector data(buffer, blen);

        TagLib::Picture pic(data,
          TagLib::Picture::FrontCover,
          mimetype,
          "Added with node-taglib2");

        picMap.insert(pic);
        hasPics = true;
      }
    }

    if (hasPics) {
      tag->setPictures(picMap);
    }
  }

  f.save();

  info.GetReturnValue().Set(Nan::True());
}

NAN_METHOD(readTagsSync) {
  Nan::HandleScope scope;
  Local<v8::Object> options;
  options = v8::Local<v8::Object>::Cast(info[1]);

  if (!info[0]->IsString()) {
    Nan::ThrowTypeError("Expected a path to audio file");
    return;
  }

  std::string audio_file = *v8::String::Utf8Value(info[0]->ToString());

  if (!isFile(audio_file.c_str())) {
    Nan::ThrowTypeError("Audio file not found");
    return;
  }

  string ext;
  const size_t pos = audio_file.find_last_of(".");

  if (pos != -1) {
    ext = audio_file.substr(pos + 1);

    for (std::string::size_type i = 0; i < ext.length(); ++i)
      ext[i] = std::toupper(ext[i]);
  }
 
  TagLib::FileRef f(audio_file.c_str());
  TagLib::Tag *tag = f.tag();
  TagLib::PropertyMap map = f.properties();

  if (!tag || f.isNull()) {
    Nan::ThrowTypeError("Could not parse file");
    return;
  }

  v8::Local<v8::Object> obj = Nan::New<v8::Object>();

  if (hasOption(options, "md5")) {
    TagLib::ByteVector &buf();

    TagLib::FileStream fs(audio_file.c_str());
    TagLib::ByteVector bytes(fs.readBlock(fs.length()));
    uint8_t result[16];

    md5((uint8_t*) bytes.data(), bytes.size(), result);

    string str;
    for (int i = 0; i < 16; i++) {
      char ch[16];
      sprintf(ch, "%2.2x", result[i]);
      str += ch;
    }
    
    obj->Set(
      Nan::New("md5").ToLocalChecked(),
      Nan::New<v8::String>(str).ToLocalChecked()
    );
  }

  !tag->title().isEmpty() && obj->Set(
    Nan::New("title").ToLocalChecked(),
    TagLibStringToString(tag->title())
  );

  !tag->artist().isEmpty() && obj->Set(
    Nan::New("artist").ToLocalChecked(),
    TagLibStringToString(tag->artist())
  );

  if (map.contains("COMPILATION")) {
    obj->Set(
      Nan::New("compilation").ToLocalChecked(),
      TagLibStringToString(map["COMPILATION"].toString(","))
    );
  }

  /* if (map.contains("ENCODER")) {
    obj->Set(
      Nan::New("encoder").ToLocalChecked(),
      TagLibStringToString(map["ENCODER"].toString(","))
    );
  }*/

  if (map.contains("ALBUMARTIST")) {
    obj->Set(
      Nan::New("albumartist").ToLocalChecked(),
      TagLibStringToString(map["ALBUMARTIST"].toString(","))
    );
  }

  if (map.contains("DISCNUMBER")) {
    obj->Set(
      Nan::New("discnumber").ToLocalChecked(),
      TagLibStringToString(map["DISCNUMBER"].toString(","))
    );
  }

  if (map.contains("TRACKNUMBER")) {
    obj->Set(
      Nan::New("tracknumber").ToLocalChecked(),
      TagLibStringToString(map["TRACKNUMBER"].toString(","))
    );
  }

  if (map.contains("BPM")) {
    TagLib::String sl = map["BPM"].toString("");
    TagLib::ByteVector vec = sl.data(TagLib::String::UTF8);
    char* s = vec.data();

    int i = 0;

    if (s) {
      i = atoi(s);
    }

    obj->Set(
      Nan::New("bpm").ToLocalChecked(),
      Nan::New<v8::Integer>(i)
    ); 
  }

  if (map.contains("COMPOSER")) {
    obj->Set(
      Nan::New("composer").ToLocalChecked(),
      TagLibStringToString(map["COMPOSER"].toString(","))
    );
  }

  !tag->album().isEmpty() && obj->Set(
    Nan::New("album").ToLocalChecked(),
    TagLibStringToString(tag->album())
  );

  !tag->comment().isEmpty() && obj->Set(
    Nan::New("comment").ToLocalChecked(),
    TagLibStringToString(tag->comment())
  );

  !tag->genre().isEmpty() && obj->Set(
    Nan::New("genre").ToLocalChecked(),
    TagLibStringToString(tag->genre())
  );

  obj->Set( // is always at least 0
    Nan::New("track").ToLocalChecked(),
    Nan::New<v8::Integer>(tag->track())
  );

  obj->Set( // is always at least 0
    Nan::New("year").ToLocalChecked(),
    Nan::New<v8::Integer>(tag->year())
  );

  // Ok, this was a quick fix. And I'll admit, opening another handle to the
  // file is not the greatest way to get the pictures for flac files. It seems
  // like this should be managed by the tag->pictures() method on FileRef, but
  // isn't, open to changes here.
  if (audio_file.find(".flac") != std::string::npos) {

    TagLib::FLAC::File flacfile(audio_file.c_str());
    TagLib::List<TagLib::FLAC::Picture *> list = flacfile.pictureList();

    size_t arraySize = list.size();
    v8::Local<v8::Array> pictures = Nan::New<v8::Array>(arraySize);

    int picIndex = 0;

    for (auto& p : list) {

      v8::Local<v8::Object> imgObj = Nan::New<v8::Object>();

      auto data = p->data();
      auto datasize = data.size();
      const char* rawdata = data.data();

      v8::Local<v8::Object> buf = Nan::NewBuffer(datasize).ToLocalChecked();
      memcpy(node::Buffer::Data(buf), rawdata, datasize);

      imgObj->Set(
        Nan::New("mimetype").ToLocalChecked(),
        TagLibStringToString(p->mimeType())
      );

      imgObj->Set(
        Nan::New("picture").ToLocalChecked(),
        buf
      );

      pictures->Set(picIndex++, imgObj);
    }

    obj->Set(Nan::New("pictures").ToLocalChecked(), pictures);
  }
  else if (!tag->pictures().isEmpty()) {

    size_t arraySize = tag->pictures().size();
    v8::Local<v8::Array> pictures = Nan::New<v8::Array>(arraySize);
    int picIndex = 0;

    for (auto& p : tag->pictures()) {

      v8::Local<v8::Object> imgObj = Nan::New<v8::Object>();

      auto data = p.second[0].data();
      auto datasize = data.size();
      const char* rawdata = data.data();

      v8::Local<v8::Object> buf = Nan::NewBuffer(datasize).ToLocalChecked();
      memcpy(node::Buffer::Data(buf), rawdata, datasize);

      imgObj->Set(
        Nan::New("mimetype").ToLocalChecked(),
        TagLibStringToString(p.second[0].mime())
      );

      imgObj->Set(
        Nan::New("picture").ToLocalChecked(),
        buf
      );

      pictures->Set(picIndex++, imgObj);
    }

    obj->Set(Nan::New("pictures").ToLocalChecked(), pictures);
  }

  if (f.audioProperties()) {

    TagLib::AudioProperties *properties = f.audioProperties();

    int seconds = properties->length() % 60;
    int minutes = (properties->length() - seconds) / 60;

    obj->Set(
      Nan::New("bitrate").ToLocalChecked(),
      Nan::New<v8::Integer>(properties->bitrate())
    );

    obj->Set(
      Nan::New("samplerate").ToLocalChecked(),
      Nan::New<v8::Integer>(properties->sampleRate())
    );

    obj->Set(
      Nan::New("channels").ToLocalChecked(),
      Nan::New<v8::Integer>(properties->channels())
    );

    // this is the same hackery, a second read, is required to get the codec
    // since codec isn't always a member of audioProperties. There should be
    // a better way of getting properties that are unique to each format.
    if (ext == "M4A" || ext == "MP4") {
      TagLib::MP4::File mp4file(audio_file.c_str());
      if (mp4file.audioProperties()) {
        auto codec = mp4file.audioProperties()->codec();

        string encoding = codec == 2 ? "alac" : "aac";

        obj->Set(
          Nan::New("codec").ToLocalChecked(),
          Nan::New<v8::String>(encoding).ToLocalChecked()
        );
      }
    }

    stringstream ss;
    ss << minutes << ":" << setfill('0') << setw(2) << seconds;
    string s = ss.str();

    auto time = Nan::New<v8::String>(s.c_str(), s.size()).ToLocalChecked();

    obj->Set(Nan::New("time").ToLocalChecked(), time);

    obj->Set(
      Nan::New("length").ToLocalChecked(),
      Nan::New<v8::Integer>(properties->length())
    );
  }

  info.GetReturnValue().Set(obj);
}

void Init(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, void *) {
  exports->Set(Nan::New("writeTagsSync").ToLocalChecked(),
    Nan::New<v8::FunctionTemplate>(writeTagsSync)->GetFunction());

  exports->Set(Nan::New("readTagsSync").ToLocalChecked(),
    Nan::New<v8::FunctionTemplate>(readTagsSync)->GetFunction());
}

NODE_MODULE(taglib2, Init)
