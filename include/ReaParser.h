#pragma once

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>
#include <vector>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace ReaParser {

	constexpr size_t ReaBuffer_Max = 1024;
	using ReaBuffer = char[ReaBuffer_Max]; // This may become a wrapper class in the future

	class Parser;
	struct ReaProject;

	// ------------ //
	// Data Structs //
	// ------------ //

	// Options to pass to parser
	struct ReaOptions {
		// By default Reaper serializes track volume by amplitude.
		// Set this true to convert all volume to decibels (as seen on track fader tooltips).
		bool ConvertVolumeToDB = true;

		// Set true, pan values will be between -1 (left) and 1 (right), as serialized.
		// Set false to range pan values between -100% and 100% (as seen on track pan tooltips).
		bool NormalizePan = true;
	};

	struct ReaVersion {
		enum class ReaPlatform {
			Undefined = 0,
			Windows, OSX, Linux
		} Platform = ReaPlatform::Undefined;

		unsigned int Major = 0, Minor = 0;

		std::string PlatformString() {
			switch (Platform) {
			case ReaPlatform::Windows: return "Windows";
			case ReaPlatform::OSX: return "Apple OSX";
			case ReaPlatform::Linux: return "Linux";
			default: return "Unknown";
			}
		}

		std::string ToString() {
			return std::string(PlatformString() + " " + std::to_string(Major) + "." + std::to_string(Minor));
		}
	};
	
	enum class ReaMediaType {
		Undefined = 0,
		Sample, Midi
	};

	struct ReaMediaItem {

		std::string Name, Filepath;
		float Volume = 0.0, Pan = 0.0;
		bool Muted = false;
		ReaMediaType Type = ReaMediaType::Undefined;

		// Start position in seconds
		float Start = 0.0f;

		// End position in seconds
		float End = 0.0f;

		// Length in seconds
		float Length = 0.0f;

		std::string ToString() {
			switch (Type) {
			case ReaMediaType::Sample: return "Sample";
			case ReaMediaType::Midi: return "Midi";
			default: return "Unknown";
			}
		}
	};
	using ReaMediaItems = std::vector<ReaMediaItem>;

	enum class ReaFXType {
		Undefined = 0,
		VST, VST3, VSTi, VST3i, AU, AUi, JS
	};

	struct ReaFX {
		std::string Name, Filepath, Data;
		ReaFXType Type;
	};
	using ReaFXChain = std::vector<ReaFX>;

	struct ReaTrack {
	public:
		friend Parser;

		std::string Name, GUID;
		float Volume = 0.0f, Pan = 0.0f;
		unsigned int NumericID = 0, Channels = 0;
		bool Muted = false;
		bool PhaseInverted = false;

		ReaMediaItems MediaItems;
		ReaFXChain FXChain;
	private:
		ReaProject* m_project;
	};
	using ReaTracks = std::vector<ReaTrack>;

	struct ReaTempo {
		unsigned int Beats = 0, Bars = 0;
		float BPM = 0.0f;
	};

	struct ReaProject {
	public:
		friend ReaProject LoadProjectFile(const char* filepath, ReaOptions options);
		friend Parser;

		std::string Name, Filepath;
		ReaVersion Version;
		ReaTracks Tracks;
		ReaTempo Tempo;
		unsigned int SampleRate = 0;

		bool IsValid() { return m_valid; }

		operator bool() { return m_valid; }
	private:
		bool m_valid = false;
		ReaOptions m_options;
	};

	// ---------- //
	// Exceptions //
	// ---------- //

	class Exception {
	public:
		Exception() = default;
		Exception(const std::string& what) { m_what = what; }

		virtual std::string What() { return m_what; }
	protected:
		std::string m_what;
	};

	class BadFile : public Exception {
	public:
		BadFile() = default;
		BadFile(const std::string& what) : Exception(what) {}
	};

	// --------- //
	// Functions //
	// --------- //

	// Utility functions
	class Util {
	public:
		Util() = delete;
		Util(const Util&) = delete;

		static inline float ToDecibel(float amplitude) { return 20.0f * log10(amplitude); }
	};

	// Core parser functions
	class Parser {
	public:
		friend ReaProject LoadProjectFile(const char* filepath, ReaOptions options);

		Parser() = delete;
		Parser(const Parser&) = delete;
	private:
		static void LoadMetadata(FILE* fp, ReaProject& project);
		static void LoadProperties(FILE* fp, ReaProject& project);
		static void LoadTracks(FILE* fp, ReaProject& project);
		static void LoadMasterTrack(FILE* fp, ReaProject& project);
		static void LoadMediaItem(FILE* fp, ReaTrack& track);
		static void LoadFX(FILE* fp, ReaTrack& track);
	};

	// Loads Reaper project data from file
	inline ReaProject LoadProjectFile(const char* filepath, ReaOptions options) {
		ReaProject project;
		project.m_options = options;

		FILE* fp = fopen(filepath, "rb");

		if (!fp) {
			throw BadFile("Unable to load Reaper project: " + std::string(filepath));
			return project;
		}
		project.Filepath = filepath;

		Parser::LoadMetadata(fp, project);
		Parser::LoadProperties(fp, project);
		Parser::LoadTracks(fp, project);

		fclose(fp);
		project.m_valid = true;
		return project;
	}

	inline void Parser::LoadMetadata(FILE* fp, ReaProject& project) {
		ReaBuffer buffer;

		// Verify valid Reaper project using the first line of the file
		if (fscanf(fp, "<REAPER_PROJECT %*f \"%i.%i/%[^\"]s\" %*i",
			&project.Version.Major, &project.Version.Minor, buffer) == 3) {
			if (strcmp(buffer, "win64") == 0 ||
				strcmp(buffer, "win32") == 0)
				project.Version.Platform = ReaVersion::ReaPlatform::Windows;
			if (strcmp(buffer, "OSX64") == 0 ||
				strcmp(buffer, "OSX32") == 0)
				project.Version.Platform = ReaVersion::ReaPlatform::OSX;
		}
		else {
			throw BadFile("Invalid Reaper project: " + project.Filepath);
			fclose(fp);
		}

		// Set name
		project.Name = project.Filepath;

		size_t startIndex = project.Name.find_last_of("/\\");
		if (startIndex != std::string::npos)
			project.Name = project.Name.substr(startIndex + 1, project.Name.size());
		size_t endIndex = project.Name.find_last_of(".");
		if (endIndex != std::string::npos)
			project.Name = project.Name.substr(0, endIndex);


		// Rewind before loading properties
		rewind(fp);
	}

	inline void Parser::LoadProperties(FILE* fp, ReaProject& project) {
		ReaBuffer buffer;
		const char* markerHeader = "  MARKER";

		while (fgets(buffer, ReaBuffer_Max, fp) != NULL) {
			sscanf(buffer, "  SAMPLERATE %i %*i %*i", &project.SampleRate);
			sscanf(buffer, "  TEMPO %f %i %i",
				&project.Tempo.BPM, &project.Tempo.Beats, &project.Tempo.Bars);
		}

		// Rewind before loading tracks
		rewind(fp);
	}

	inline void Parser::LoadTracks(FILE* fp, ReaProject& project) {
		// Initialize Master track. It will always be at the 0th index!
		LoadMasterTrack(fp, project);

		ReaBuffer buffer;
		int trackCount = 0;
		const char* trackFooter = "  >";
		const char* itemHeader = "    <ITEM";

		// Scan entire file for tracks
		// Using the XOR operator with scanset specifier is helpful here
		while (fgets(buffer, ReaBuffer_Max, fp) != NULL) {
			
			// Track found, begin parsing
			if (sscanf(buffer, "  <TRACK {%[^}]s}", &buffer) == 1) {
				ReaTrack track;
				track.m_project = &project;
				track.GUID = buffer;
				track.NumericID = ++trackCount;

				const char* fxChainHeader = "    <FXCHAIN";

				// Read from track data
				while (fgets(buffer, ReaBuffer_Max, fp) != NULL) {
					if (strncmp(buffer, trackFooter, strlen(trackFooter)) == 0)
						break; // Track footer hit

					if (sscanf(buffer, "    NAME \"%[^\"]s\"", buffer) == 1 ||
						sscanf(buffer, "    NAME %s", buffer) == 1)
						track.Name = buffer;
					sscanf(buffer, "    VOLPAN %f %f %*i %*i %*i",
						&track.Volume, &track.Pan);
					int invPhase = 0;
					if (sscanf(buffer, "    IPHASE %i", &invPhase) == 1) {
						track.PhaseInverted = invPhase;
					}
					int muted = 0;
					if (sscanf(buffer, "    MUTESOLO %i %*i %*i", &muted) == 1) {
						track.Muted = muted;
					}

					// Load MediaItem
					if (strncmp(buffer, itemHeader, strlen(itemHeader)) == 0)
						LoadMediaItem(fp, track);

					// Load FX
					if (strncmp(buffer, fxChainHeader, strlen(fxChainHeader)) == 0) {
						LoadFX(fp, track);
					}
				}

				// Apply options
				ReaOptions options = track.m_project->m_options;

				if (options.ConvertVolumeToDB)
					track.Volume = Util::ToDecibel(track.Volume);
				if (!options.NormalizePan)
					track.Pan *= 100.0f;

				project.Tracks.push_back(track);
			}
		}
	}

	inline void Parser::LoadMasterTrack(FILE* fp, ReaProject& project) {
		ReaBuffer buffer;
		ReaTrack master;
		master.m_project = &project;
		master.GUID = "0";
		master.Name = "MASTER";

		while (fgets(buffer, ReaBuffer_Max, fp) != NULL) {
			// Second digit is output channels
			sscanf(buffer, "  MASTER_NCH %*i %i", &master.Channels);
			sscanf(buffer, "  MASTER_VOLUME %f %f %*i %*i %*i",
				&master.Volume, &master.Pan);
		}		
		
		// Apply options
		ReaOptions options = project.m_options;

		if (options.ConvertVolumeToDB)
			master.Volume = Util::ToDecibel(master.Volume);
		if (!options.NormalizePan)
			master.Pan *= 100.0f;

		project.Tracks.push_back(master);

		// Rewind before returning to load tracks
		rewind(fp);
	}

	inline void Parser::LoadMediaItem(FILE* fp, ReaTrack& track) {
		ReaBuffer buffer;
		ReaMediaItem item;
		const char* itemFooter = "    >";
		const char* midiHeader = "      <SOURCE MIDI";
		const char* waveHeader = "      <SOURCE WAVE";
		const char* mp3Header  = "      <SOURCE MP3";

		while (fgets(buffer, ReaBuffer_Max, fp) != NULL) {
			if (strncmp(buffer, itemFooter, strlen(itemFooter)) == 0)
				break;

			sscanf(buffer, "      POSITION %f", &item.Start);
			sscanf(buffer, "      LENGTH %f", &item.Length);
			sscanf(buffer, "      MUTE %i %*i", &item.Muted);
			if (sscanf(buffer, "      NAME \"%[^\"]s\"", buffer) == 1 ||
				sscanf(buffer, "      NAME %s", buffer) == 1)
				item.Name = buffer;
			sscanf(buffer, "      VOLPAN %f %f %*f %*f",
				&item.Volume, &item.Pan);

			if (strncmp(buffer, midiHeader, strlen(midiHeader)) == 0)
				item.Type = ReaMediaType::Midi;
			if (strncmp(buffer, waveHeader, strlen(waveHeader)) == 0 ||
				strncmp(buffer, mp3Header, strlen(mp3Header)) == 0) {
				item.Type = ReaMediaType::Sample;
				// Advance to next line and attempt to grab filepath
				fgets(buffer, ReaBuffer_Max, fp);
				if (sscanf(buffer, "        FILE \"%[^\"]s\" %*f", &buffer) == 1)
					item.Filepath = buffer;
			}
		}

		// Apply options
		ReaOptions options = track.m_project->m_options;

		if (options.ConvertVolumeToDB)
			item.Volume = Util::ToDecibel(item.Volume);
		if (!options.NormalizePan)
			item.Pan *= 100.0f;

		item.End = item.Start + item.Length;
		track.MediaItems.push_back(item);
	}

	inline void Parser::LoadFX(FILE* fp, ReaTrack& track) {
		ReaBuffer buffer, fxTypeName, fxName, fxFile;
		const char* fxChainFooter = "    >";
		const char* fxFooter = "      >";

		while (fgets(buffer, ReaBuffer_Max, fp) != NULL) {
			if (strncmp(buffer, fxChainFooter, strlen(fxChainFooter)) == 0)
				break;

			// Standard FX
			if (sscanf(buffer, "      <%*s \"%[^:]: %[^\"]\" %s 0 \"\" %*i<%*32s> \"\"", 
				fxTypeName, fxName, fxFile) == 3) {
				ReaFX fx;
				
				fx.Name = fxName;
				fx.Filepath = fxFile;
				if (strncmp(fxTypeName, "VST", 3) == 0)
					fx.Type = ReaFXType::VST;
				else if (strncmp(fxTypeName, "VST3", 4) == 0)
					fx.Type = ReaFXType::VST3;
				else if (strncmp(fxTypeName, "VSTi", 4) == 0)
					fx.Type = ReaFXType::VSTi;
				else if (strncmp(fxTypeName, "VST3i", 5) == 0)
					fx.Type = ReaFXType::VST3i;
				else if (strncmp(fxTypeName, "AU", 2) == 0)
					fx.Type = ReaFXType::AU;
				else if (strncmp(fxTypeName, "AUi", 3) == 0)
					fx.Type = ReaFXType::AUi;

				// Grab data strings from FX
				while (fgets(buffer, ReaBuffer_Max, fp) != NULL) {
					if (strncmp(buffer, fxFooter, strlen(fxFooter)) == 0)
						break; // FX footer hit
					
					fx.Data.append(buffer);
				}

				// Strip any tab, space and Windows \r characters from data string
				fx.Data.erase(std::remove(fx.Data.begin(), fx.Data.end(), '\t'), fx.Data.cend());
				fx.Data.erase(std::remove(fx.Data.begin(), fx.Data.end(), ' '), fx.Data.cend());
				fx.Data.erase(std::remove(fx.Data.begin(), fx.Data.end(), '\r'), fx.Data.cend());

				track.FXChain.push_back(fx);
			}

			// JesuSonic FX
			if (strncmp(buffer, "      <JS ", 10) == 0) {
				sscanf(buffer, "      <JS %s \"\"", fxName);
				ReaFX jsFx;
				jsFx.Name = fxName;
				jsFx.Type = ReaFXType::JS;

				// Just grab the next line's data string
				fgets(buffer, ReaBuffer_Max, fp);
				jsFx.Data = buffer;

				// And then remove leading spaces
				for (auto it = jsFx.Data.begin(); it != jsFx.Data.end(); ++it) {
					char c = *it;

					if (c != ' ') {
						jsFx.Data.erase(jsFx.Data.begin(), it);
						break;
					}
				}

				track.FXChain.push_back(jsFx);
			}
		}
	}
}