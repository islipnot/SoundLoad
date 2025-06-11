# SoundLoad
The goal of this project is to help all SoundCloud users save their favorite songs 
with as much data as possible preserved. Spotify users also benefit from this, as 
they can effortlessly import a song to local files in seconds, where it would otherwise 
take several minutes if the song was downloaded and the metadata was all set manually.

## Usage
A SoundCloud client ID is required for every use unless saved to `cfg.json`.
</br> ( <i>Use `-CID` & `-save` to store a CID</i> )

### Getting a new client ID
- Open browser dev tools (`ctrl+shift+i`)
- Go to the network tab
- Go to [SoundCloud](https://soundcloud.com)
- Filter URL's with "client_id="
- Find a request with a client ID in it

### Arguments (case-insensitive)

| Argument     | Description                                                         | Saveable? | Album/Playlist? |
|--------------|---------------------------------------------------------------------|-----------|-----------------|
| `-CID`       | SoundCloud client ID                                                | ✅        | ✅             |
| `-PVar`      | Adds program to user PATH variables                                 | ❌        | ✅             |
| `-save`      | Saves applicable arguments to `cfg.json`                            | ❌        | ✅             |
| `-cFile`     | Cover art file name                                                 | ❌        | ✅             |
| `-cDst`      | Cover art output directory                                          | ✅        | ✅             |
| `-cSrc`      | Cover art source (path, SoundCloud link, image link)                | ❌        | ✅             |
| `-art`       | Independent cover art download                                      | ✅        | ✅             |
| `--art`      | Disable independent cover art download                              | ✅        | ✅             |
| `-aFile`     | MP3 file name                                                       | ❌        | ✅             |
| `-aDst`      | MP3 output directory                                                | ✅        | ✅             |
| `--audio`    | Prevents audio from being downloaded                                | ✅        | ✅             |
| `-audio`     | Re-enables audio download (undoes `--audio`)                        | ✅        | ✅             |
| `-title`     | ID3v2 title                                                         | ❌        | ❌             |
| `-comment`   | ID3v2 comment                                                       | ❌        | ❌             |
| `-cArtist`   | ID3v2 contributing artists                                          | ❌        | ❌             |
| `-aArtist`   | ID3v2 album artist                                                  | ❌        | ✅             |
| `-album`     | ID3v2 album                                                         | ❌        | ❌             |
| `-genre`     | ID3v2 genre                                                         | ❌        | ❌             |
| `-tNum`      | ID3v2 track number                                                  | ❌        | ❌             |
| `-year`      | ID3v2 year                                                          | ❌        | ✅             |


## Examples

### Setting basic config info
By running this, the client ID, MP3 output dir, and cover art output dir will be saved to cfg.json. Note that creating
a local files folder for spotify like shown in this example makes importing songs to spotify extremely quick.
```
c:>soundload -cid cWww6yL0wMOcwhn4GEYjHVAg3mwMPBis -adst "c:/spotify local files" -cdst "c:/cover art" -save
```

### Downloading a song
This will download the song, name the MP3 file "like a stone", and set the contributing artists to "Audioslave".
Other metadata will be automatically scraped from the page, but can of course be set manually.
```
c:>soundload https://soundcloud.com/bezz_records/audioslave-like-a-stone -afile "like a stone" -cartist Audioslave
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
underground music, where cover art is often changed or lost to time. The `--audio` arg is also used 
and saved, preventing an MP3 from being downloaded.
```
c:>soundload https://soundcloud.com/hexxarchive_4/pint-x-4jay-fuck12-prod-mexikodro --audio -art -save

c:>soundload https://soundcloud.com/ebkyoungjoc-sc/need-love
```

## Notes
- Unicode characters such as emojis are not handled on command line
- The following characters will be replaced with an underscore in file names: `< > : " \ | ? *`
- Go+ songs cannot be downloaded.