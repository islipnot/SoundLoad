# SoundLoad
The goal of this project is to help all SoundCloud users save their favorite songs with as much data as possible preserved. Spotify users also benefit from this, as they can effortlessly import a song to local files in seconds, where it would otherwise take several minutes if the song was downloaded and the metadata was all set manually.

## Arguments
If you wish to download a SoundCloud track or track cover, you must provide a link to it as the first argument.
- ```-CID     <client ID>```
- ```-fName   <MP3 file name&>```
- ```-title   <MP3 title property>```
- ```-artists <MP3 contributing artists property>```
- ```-artist  <MP3 album artist property>```
- ```-album   <MP3 album property>```
- ```-genre   <MP3 genre property>```
- ```-year    <MP3 year property>```
- ```-num     <MP3 # property>```
- ```-out     <Output directory>```
- ```-save    (save to cfg.txt)```

## Usage
When you first use SoundLoad, you must provide a client ID, which will be saved to cfg.txt if you use the ```-save``` argument.

### Getting a new client ID
- Open browser dev tools (ctrl+shift+i, usually)
- Go to the network tab
- Go to [SoundCloud](https://soundcloud.com)
- Filter URL's with "client_id="
- Find a request with a client ID in it