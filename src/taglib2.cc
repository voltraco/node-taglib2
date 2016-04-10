#include <errno.h>
#include <string.h>

#include <v8.h>
#include <nan.h>
#include <node_buffer.h>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <tag.h>
#include <tlist.h>
#include <fileref.h>
#include <tfile.h>
#include <tpicturemap.h>
#include <tpropertymap.h>
#include <tbytevector.h>
#include <tbytevectorlist.h>

//#include <mp4tag.h>
//#include <mp4atom.h>
//#include <mp4file.h>

using namespace std;
using namespace v8;

bool isFile(const char *s) {
  struct stat st;
#ifdef _WIN32
  return ::stat(s, &st) == 0 && (st.st_mode & (S_IFREG));
#else
  return ::stat(s, &st) == 0 && (st.st_mode & (S_IFREG | S_IFLNK));
#endif
}

void checkForRejectedProperties(const TagLib::PropertyMap &tags) {
  if(tags.size() > 0) {
    unsigned int longest = 0;
    for(TagLib::PropertyMap::ConstIterator i = tags.begin(); i != tags.end(); ++i) {
      if(i->first.size() > longest) {
        longest = i->first.size();
      }
    }
    //cout << "-- rejected TAGs (properties) --" << endl;
    for(TagLib::PropertyMap::ConstIterator i = tags.begin(); i != tags.end(); ++i) {
      for(TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j) {
        // TODO build and return a JSON object
        //cout << left << std::setw(longest) << i->first << " - " << '"' << *j << '"' << endl;
      }
    }
  }
}

Local<Value> TagLibStringToString(TagLib::String s) {

  if (s.isEmpty()) return Nan::Null();

  TagLib::ByteVector str = s.data(TagLib::String::UTF16);

  return Nan::New<v8::String>(
    (uint16_t *) str.mid(2, str.size()-2).data(),
    s.size()
  ).ToLocalChecked();
}

NAN_METHOD(writeTagsSync) {
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

  if (!tag) {
    Nan::ThrowTypeError("Could not parse file");
    return;
  }

  auto hasOption = [&](const std::string name) -> bool {
    return options->Has(Nan::New(name).ToLocalChecked());
  };

  auto getOptionString = [&](const std::string name) -> std::string {
    auto r = options->Get(Nan::New(name).ToLocalChecked());
    std::string s = *v8::String::Utf8Value(r);
    return s.c_str();
  };

  auto getOptionInt = [&](const std::string name) -> int {
    return options->Get(Nan::New(name).ToLocalChecked())->Int32Value();
  };

  if (hasOption("artist")) tag->setArtist(getOptionString("artist"));
  if (hasOption("title")) tag->setTitle(getOptionString("title"));
  if (hasOption("album")) tag->setAlbum(getOptionString("album"));
  if (hasOption("comment")) tag->setComment(getOptionString("comment"));
  if (hasOption("genre")) tag->setGenre(getOptionString("genre"));
  if (hasOption("year")) tag->setYear(getOptionInt("year"));
  if (hasOption("track")) tag->setTrack(getOptionInt("track"));

  auto cover = options->Get(Nan::New("cover").ToLocalChecked());

  if (!cover.IsEmpty() && node::Buffer::HasInstance(cover->ToObject())) {

    char* buffer = node::Buffer::Data(cover->ToObject());
    const size_t blen = node::Buffer::Length(cover->ToObject());
    TagLib::ByteVector data(buffer, blen);

    if (!data.find("JFIF")) {
      Nan::ThrowTypeError("Image is empty or not a JPEG");
      return;
    }

    TagLib::Picture pic(data,
      TagLib::Picture::FrontCover,
      "image/jpeg",
      "Added with node-taglib2");

    TagLib::PictureMap picMap(pic);
    tag->setPictures(picMap);
  }

  f.save();

  info.GetReturnValue().Set(Nan::True());
}

NAN_METHOD(readTagsSync) {

  if (!info[0]->IsString()) {
    Nan::ThrowTypeError("Expected a path to audio file");
    return;
  }

  std::string audio_file = *v8::String::Utf8Value(info[0]->ToString());

  if (!isFile(audio_file.c_str())) {
    Nan::ThrowTypeError("Audio file not found");
    return;
  }

  TagLib::FileRef f(audio_file.c_str());
  TagLib::Tag *tag = f.tag();

  if (!tag || f.isNull()) {
    Nan::ThrowTypeError("Could not parse file");
    return;
  }

  v8::Local<v8::Object> obj = Nan::New<v8::Object>();

  !tag->title().isEmpty() && obj->Set(
    Nan::New("title").ToLocalChecked(),
    TagLibStringToString(tag->title())
  );

  !tag->artist().isEmpty() && obj->Set(
    Nan::New("artist").ToLocalChecked(),
    TagLibStringToString(tag->artist())
  );

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

  if (!tag->pictures().isEmpty()) {

    size_t arraySize = tag->pictures().size();
    v8::Local<v8::Array> pictures = Nan::New<v8::Array>(arraySize);
    int picIndex = 0;

    for (auto& p : tag->pictures()) {
  
      auto data = p.second[0].data();
      auto datasize = data.size();
      const char* rawdata = data.data();

      v8::Local<v8::Object> buf = Nan::NewBuffer(datasize).ToLocalChecked();
      memcpy(node::Buffer::Data(buf), rawdata, datasize);

      pictures->Set(picIndex++, buf);
    }

    obj->Set(Nan::New("pictures").ToLocalChecked(), pictures);
  }

  if(f.audioProperties()) {

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

void Init(v8::Local<v8::Object> exports) {
  exports->Set(Nan::New("writeTagsSync").ToLocalChecked(),
    Nan::New<v8::FunctionTemplate>(writeTagsSync)->GetFunction());

  exports->Set(Nan::New("readTagsSync").ToLocalChecked(),
    Nan::New<v8::FunctionTemplate>(readTagsSync)->GetFunction());
}

NODE_MODULE(taglib2, Init)

