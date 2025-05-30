# SoundLoad
The goal of this project is to help all SoundCloud users save their favorite songs 
with as much data as possible preserved. Spotify users also benefit from this, as 
they can effortlessly import a song to local files in seconds, where it would otherwise 
take several minutes if the song was downloaded and the metadata was all set manually.

# Usage
You must provide a client ID every use unless you save one to cfg.json. This can be done 
by using the `-CID` & `-save` arguments in conjunction.

### Getting a new client ID
- Open browser dev tools (ctrl+shift+i, usually)
- Go to the network tab
- Go to [SoundCloud](https://soundcloud.com)
- Filter URL's with "client_id="
- Find a request with a client ID in it

### Arguments (optional)
- `-cid` – SoundCloud client ID (saveable)
- `-PVar` – Adds program to user PATH variables
- `-save` – Saves applicable args to cfg.json
- `-cFile` – Cover art file name
- `-cDst` – Cover art output directory (saveable)
- `-art` – Downloads cover art separately from MP3 (saveable)
- `--art` – Disables separate cover art download (saveable)
- `-aFile` – MP3 file name
- `-aDst` – MP3 output directory (saveable)
- `--audio` – Audio will not be downloaded (saveable)
- `-audio` – Reverses the effect of --audio (saveable)
- `-title` – MP3 title property
- `-comment` – MP3 comment property
- `-cArtist` – MP3 contributing artists property
- `-aArtist` – MP3 album artist property
- `-album` – MP3 album property
- `-genre` – MP3 genre property
- `-tNum` – MP3 track number property
- `-year` – MP3 year property

### Downloading a track/album/playlist
- Put the SoundCloud link as the first argument
- Optionally, add any combination of the arguments listed above
- After running, the MP3(s) will be in the provided output directory, or if none was provided, in the executable directory

## Features
- Downloads tracks at the highest available quality. Progressive transcodings are
preferred, with HLS MPEG transcodings being used only if progressive isn't available.
- Automatically scrapes metadata and fills out ID3v2 tags. This includes the cover art at the 
highest quality.

## Notes
- Non-ASCII characters are supported in every case except command line arguments. This is subject to change.
- The following characters will be replaced with an underscore in file names: `< > : " \ | ? *`
- Go+ songs cannot be downloaded.