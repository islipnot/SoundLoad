# SoundLoad
The goal of this project is to help all SoundCloud users save their favorite songs with as much data as possible preserved. Spotify users also benefit from this, as they can effortlessly import a song to local files in seconds, where it would otherwise take several minutes if the song was downloaded and the metadata was all set manually.

# Usage
You must provide a client ID every use unless you save one to cfg.json. This can be done by using the ```-CID``` & ```-save``` arguments in conjunction.

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
- ```-save    (save to cfg.json)```
- ```-EnvVar  (add SoundLoad to PATH variables)```

### Downloading a track/album/playlist
- Put the SoundCloud link as the first argument
- Optionally, add any combination of the arguments listed above
- After running, the MP3(s) will be in the provided output directory, or if none was provided, in the executable directory


## Tips
- If you want to have more control over the MP3 metadata/name/path, you can use any combination of arguments listed above. If ```-save``` is used, all that will be saved (if provided) is the CID and output directory.
- When first ran, or when the "ran" field of cfg.json is false, you will have the option to add the program to your PATH variables. Using the ```-EnvVar``` argument forces the program to do this.

## Notes
- Non-ASCII characters are supported in every case except command line arguments. This is subject to change.
- The following characters will be replaced with an underscore in file names: <, >, :, ", \, |, ?, *
- As it stands, I have not added the ability to download Go+ songs, though when I do, you must have an account with Go+.