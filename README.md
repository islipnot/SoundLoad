# SoundLoad
The goal of this project is to help all SoundCloud users save their favorite songs with as much data as possible preserved. Spotify users also benefit from this, as they can effortlessly import a song to local files in seconds, where it would otherwise take several minutes if the song was downloaded and the metadata was all set manually.

# Usage
You must provide a client ID every use unless you save one to cfg.txt. This can be done by using the ```-CID``` & ```-save``` arguments in conjunction.

### Getting a new client ID
- Open browser dev tools (ctrl+shift+i, usually)
- Go to the network tab
- Go to [SoundCloud](https://soundcloud.com)
- Filter URL's with "client_id="
- Find a request with a client ID in it

### Arguments (optional)
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

### Downloading a song
- Put the link to the track as first argument, all link formats work
- Done! It's that simple!

If you want to have more control over the MP3 metadata/name/path, you can use any combination of arguments listed above. If ```-save``` is used, all that will be saved (if provided) is the CID and output directory.