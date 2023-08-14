# ReaParser
Header-only C++11 file parser for [Reaper](https://www.reaper.fm/) project files. 

## Usage

### Load project in memory
```c++
ReaParser::ReaProject project;
ReaParser::ReaOptions options;
options.NormalizePan = false;

try {
  project = ReaParser::LoadProjectFile("TestProject/TestProject.rpp", options);
}
catch (ReaParser::Exception& e) {
  std::cout << e.What() << std::endl;
}
```
### Access project properties
```c++
std::cout << "Reaper project: " << project.Name << std::endl;
std::cout << "Reaper version: " << project.Version.ToString() << std::endl;
std::cout << "Sample rate: " << project.SampleRate << "kHz" << std::endl;
std::cout << "Tempo: " << project.Tempo.BPM << " bpm " << project.Tempo.Beats << "/" << project.Tempo.Bars << std::endl << std::endl;
```
### Access tracks from project
```c++
if (project.Tracks.size() > 0) {
  for (auto& track : project.Tracks) {
    std::cout << track.NumericID << ") " << track.Name << " (" << track.GUID << ")\n";
    std::cout << "Volume: " << track.Volume << "dB Pan: " << track.Pan << "%\n";
    std::cout << "Muted: " <<
      (track.Muted ? "Yes" : "No") << std::endl;
    std::cout << "Phase: " <<
      (track.PhaseInverted ? "Flipped" : "Normal") << std::endl;
  }
}
```
### Access media items from track
```c++
for (auto& track : project.Tracks) {
  if (track.MediaItems.size() > 0) {
    std::cout << "Items: ---------------------" << std::endl;
    for (auto& item : track.MediaItems) {
      std::cout << "\"" << item.Name << "\"" << std::endl;
      if (item.Type == ReaParser::ReaMediaItem::MediaType::Sample)
          std::cout << "FILE  : " << item.Filepath << std::endl;
      std::cout << "START : " << item.Start << "s" << std::endl;
      std::cout << "END   : " << item.End << "s" << std::endl;
      std::cout << "LENGTH: " << item.Length << "s" << std::endl;
    }
  }
}
```
