# SoundLoad
The goal of this project is to help all SoundCloud users save their favorite songs 
with as much data as possible preserved. Spotify users also benefit from this, as 
they can effortlessly import a song to local files in seconds, where it would otherwise 
take several minutes if the song was downloaded and the metadata was all set manually.

## Usage
A SoundCloud client ID is required for every use unless saved to `cfg.json`.
</br> ( *Use `-CID` & `-save` to store a CID* )

### Getting a new client ID
- Open browser dev tools (`ctrl+shift+i`)
- Go to the network tab
- Go to [SoundCloud](https://soundcloud.com)
- Filter URL's with "client_id="
- Find a request with a client ID in it

### Arguments (case-insensitive)

| Argument      | Description                                          | Saveable? | Album/Playlist? |
| ------------- | ---------------------------------------------------- | --------- | --------------- |
| `-cid`        | SoundCloud client ID                                 | ✅        | ✅             |
| `-pvars`      | Adds program to user PATH variables                  | ❌        | ✅             |
| `-save`       | Saves applicable arguments to `cfg.json`             | ❌        | ✅             |
| `-img-name`   | Cover art file name                                  | ❌        | ✅             |
| `-img-dst`    | Cover art output directory                           | ✅        | ✅             |
| `-img-src`    | Cover art source (path, SoundCloud link, image link) | ❌        | ✅             |
| `-art`        | Independent cover art download                       | ✅        | ✅             |
| `-n-art`      | Disable independent cover art download               | ✅        | ✅             |
| `-audio-name` | Audio file name                                      | ❌        | ✅             |
| `-audio-dst`  | Audio output directory                               | ✅        | ✅             |
| `-audio`      | Enables audio downloads                              | ✅        | ✅             |
| `-n-audio`    | Disables MP3 downloads                               | ✅        | ✅             |
| `-aac`        | Enables lossless downloads                           | ✅        | ✅             |
| `-n-aac`      | Disables lossless downloads                          | ✅        | ✅             |
| `-title`      | Audio tag title                                      | ❌        | ❌             |
| `-comment`    | Audio tag comment                                    | ❌        | ❌             |
| `-artists`    | Audio tag contributing artists                       | ❌        | ❌             |
| `-a-artist`   | Audio tag album artist                               | ❌        | ✅             |
| `-album`      | Audio tag album                                      | ❌        | ❌             |
| `-genre`      | Audio tag genre                                      | ❌        | ❌             |
| `-num`        | Audio tag track number                               | ❌        | ❌             |
| `-year`       | Audio tag year                                       | ❌        | ✅             |


## Examples

### Setting basic config info
By running this, the client ID, audio output dir, and cover art output dir will be saved to cfg.json. Note that creating
a local files folder for spotify like shown in this example makes importing songs to spotify extremely quick.
```
c:>soundload -cid cWww6yL0wMOcwhn4GEYjHVAg3mwMPBis -audio-dst "c:/spotify local files" -img-dst "c:/cover art" -save
```

### Downloading a song
This will download the song, name the audio file "Extra Stixx", and set the contributing artists to "Pook G, Lul Jody".
Other metadata will be automatically scraped from the page, but can of course be set manually.
```
c:>soundload https://soundcloud.com/fat-kid-915108395/exrta-stixxs-ft-pook-g-lul -audio-name "Extra Stixx" -artists "Pook G, Lul Jody"
```

### Downloading an album
Songs are downloaded in order of last to first, with track numbers automatically parsed. If you want more 
control over each track in the album, download them independently.
```
c:>soundload https://soundcloud.com/axxturel/sets/s-kkkult-s-kkkult-s-kkkkult
```

### Downloading cover art
By using the `-art` arg, you tell the program to seperately download the cover art from the track. It 
should be noted that this option can be saved to cfg.json. This is useful for anyone archiving 
underground music, where cover art is often changed or lost to time. The `-n-audio` arg is also used 
and saved, preventing an MP3 from being downloaded.
```
c:>soundload https://soundcloud.com/sellasouls/bdayy-sexxx -n-audio -art -save
```

## Notes
- Lossless downloads can be enabled with the -aac command, resulting in a .m4a file type. The reason 
  this is disabled by default is because Spotify won't properly play it, which defeats the entire purpose 
  if you're downloading to local files.
- `nlohmann::json::value` has multiple modifications in this repo - check `pch.hpp` for more info.
- There is full unicode support both on command line and in parsing metadata, though it's tedious to 
  actually get proper encoding on command line in Windows.
- In the `comments` property of downloaded audio tags, the program will store the exact timestamp 
  of the upload, the original description, and the original tags. You can easily add on to this list 
  in `site_api.cpp/sc_upload::sc_upload()` by using the `add_comment` lambda.
- The following characters will be replaced with an underscore in file names: `< > : " \ | ? *`
- Go+ songs cannot be downloaded.
- `cfg.json` must not be modified manually for now. The json library I'm using doesn't have std::wstring
  support, causing it to store such types in extremely stupid ways. Only touch if you know what you're doing.
  This will be fixed later.