# ReaParser
Header-only C++11 file parser for [Reaper](https://www.reaper.fm/) project files. 

Reverse engineering of Reaper project files is done by trial and error. ReaParser is experimental, some results may be unexpected. 

## Features
ReaParser can extract the following information from a Reaper project file:

### Projects:
+ Filename
+ Reaper version and OS saved with
+ Sample Rate
+ Master tempo and time signature

### Tracks:
+ Name
+ Volume/Pan
+ Media Items
+ FX Chain

### Media Items:
+ Name
+ Associated filepath and file format
+ Start and end positions
+ Length

### FX Plugins:
+ Name
+ Associated filepath and file format
+ Data string

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
      std::cout << "START : " << item.Start << "s" << std::endl;
      std::cout << "END   : " << item.End << "s" << std::endl;
      std::cout << "LENGTH: " << item.Length << "s" << std::endl;
    }
  }
}
```

## Todo
+ Project preferences
+ Automation
+ Regions
+ More Media Item details