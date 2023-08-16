#include "../include/ReaParser.h"

#include <iostream>

int main(int argc, const char* argv[]) {
	ReaParser::ReaProject project;
	ReaParser::ReaOptions options;
	options.ConvertVolumeToDB = true;
	options.NormalizePan = false;

	try {
		project = ReaParser::LoadProjectFile("testing/TestProject/TestProject.rpp", options);
	}
	catch (ReaParser::Exception& e) {
		std::cout << e.What() << std::endl;
		exit(-1);
	}


	std::cout << "Reaper project: " << project.Name << std::endl;
	std::cout << "Reaper version: " << project.Version.ToString() << std::endl;
	std::cout << "Sample rate: " << project.SampleRate << "kHz" << std::endl;
	std::cout << "Tempo: " << project.Tempo.BPM << " bpm " << project.Tempo.Beats << "/" << project.Tempo.Bars << std::endl << std::endl;

	if (project.Tracks.size() > 0) {
		std::cout << "Tracks: " << std::endl << "----------------------------" << std::endl;
		for (auto& track : project.Tracks) {
			std::cout << track.NumericID << ") " << track.Name << " (" << track.GUID << ")\n";
			std::cout << "Volume: " << track.Volume << "dB Pan: " << track.Pan << "%\n";
			std::cout << "Muted: " <<
				(track.Muted ? "Yes" : "No") << std::endl;
			std::cout << "Phase: " <<
				(track.PhaseInverted ? "Flipped" : "Normal") << std::endl;

			if (track.MediaItems.size() > 0) {
				std::cout << "Items: ---------------------" << std::endl;
				for (auto& item : track.MediaItems) {
					std::cout << "\"" << item.Name << "\"" << std::endl;
					if (item.Type == ReaParser::ReaMediaType::Sample)
						std::cout << "FILE  : " << item.Filepath << std::endl;
					std::cout << "START : " << item.Start << "s" << std::endl;
					std::cout << "END   : " << item.End << "s" << std::endl;
					std::cout << "LENGTH: " << item.Length << "s" << std::endl;
				}
			}

			if (track.FXChain.size() > 0) {
				std::cout << "FX Chain: ------------------" << std::endl;
				for (auto& fx : track.FXChain) {
					std::cout << fx.Name << " (" << fx.Filepath << ")\n";
					std::cout << fx.Data << std::endl; 
				}
			}

			std::cout << std::endl;
		}
	}
	
	return 0;
}