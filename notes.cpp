
/***************************************************************************
 *   notes.cpp  --  This file is part of notes_tutor.                      *
 *                                                                         *
 *   Copyright (C) 2024 Imanol-Mikel Barba Sabariego                       *
 *                                                                         *
 *   notes_tutor is free software: you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published     *
 *   by the Free Software Foundation, either version 3 of the License,     *
 *   or (at your option) any later version.                                *
 *                                                                         *
 *   notes_tutor is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty           *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
 *   See the GNU General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.   *
 *                                                                         *
 ***************************************************************************/

#include <iomanip>
#include <iostream>
#include <random>

#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <gflags/gflags.h>
#include <rtmidi/RtMidi.h>

#include "scope_guard.hpp"

#define DIFFICULTY_EASY 1
#define DIFFICULTY_MED 2
#define DIFFICULTY_HARD 3

#define CHANNEL_1_NOTE_ON 0x90

// Initialise PRNG
std::random_device seed;
std::mt19937 gen{seed()};
std::uniform_int_distribution<> dist{21, 108};

volatile bool done = false;
void sigint_handler(int sig) {
  done = true;
}

std::wstring get_note_name(unsigned int note, const std::string& notation) {
  std::wstring note_names_english[12] = {L"C", L"C#", L"D", L"D#", L"E", L"F", L"F#", L"G", L"G#", L"A", L"A#", L"B"};
  std::wstring note_names_solfege[12] = {L"Do", L"Do#", L"Re", L"Re#", L"Mi", L"Fa", L"Fa#", L"Sol", L"Sol#", L"La", L"La#", L"Ti"};
  // U+2080 is the base subscript number (0)
  wchar_t subscript = 0x2080 + ((note - 12) / 12);

  if(notation == "solfege") {
    return note_names_solfege[(note - 12) % 12] + std::wstring(&subscript, 1);
  }
  return note_names_english[(note - 12) % 12] + std::wstring(&subscript, 1);
}

unsigned int get_random_note(unsigned int difficulty) {
  // In order to avoid black keys, we could:
  //   - Generate a random note, and iterate until a suitable one appears
  //   - Generate a random note, and add or substract one if it falls on a
  //     black key
  //   - Generate a valid set of only white keys, and generate a random index
  // Ideally we'd do #3 for speed and fairness, but I'm lazy. #2 is fastest
  // but creates bias (some keys are more statistically frequent), so we're
  // going with #1 which is slower, but it should be okay since not many
  // iterations will be needed as white keys are more frequent than black keys

  unsigned int note = dist(gen);
  // If medium or easy, omit black keys
  if(difficulty <= DIFFICULTY_MED) {
    unsigned int note_letter = (note - 12) % 12;
    while(note_letter == 1 || note_letter == 3 | note_letter == 6 | note_letter == 8 | note_letter == 10) {
      note = dist(gen);
      note_letter = (note - 12) % 12;
    }
  }

  // If easy, convert the octave to the 4th one only
  if(difficulty == DIFFICULTY_EASY) {
    unsigned int note_letter = (note - 12) % 12;
    note = (note_letter + 12) + (12 * 4);
  }

  return note;
}

DEFINE_string(difficulty, "easy", "Difficulty level. 'easy' is only 4th octave, no black keys. 'med' is no black keys. 'hard' is any key.");
DEFINE_bool(forward, false, "Enable forwarding MIDI inputs to another MIDI device. The specific device is selected interactively.");
DEFINE_string(notation, "english", "Musical note notation, either 'english' or 'solfege'.");
DEFINE_uint64(channel, 1, "MIDI input channel number.");

int main(int argc, char** argv) {
  gflags::SetVersionString("1.0.0");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  struct sigaction act;
  memset(&act, 0, sizeof(act));

  /* Set up the structure to specify the new action. */
  act.sa_handler = sigint_handler;
  sigemptyset(&act.sa_mask);

  if(sigaction(SIGINT, &act, NULL) == -1) {
    std::cerr << "Unable to set SIGINT handler" << std::endl;
    exit(1);
  }

  RtMidiIn* midi_in = nullptr;
  RtMidiOut* midi_out = nullptr;

  // RtMidiIn constructor
  try {
    midi_in = new RtMidiIn();
  } catch (RtMidiError& error) {
    error.printMessage();
    return 1;
  }

  ScopeGuard midi_in_guard([midi_in]() {
    delete midi_in;
  });

  unsigned int num_ports = midi_in->getPortCount();
  std::cout << num_ports << " MIDI input sources available" << std::endl;
  std::string port_name;
  for(unsigned int i = 0; i < num_ports; ++i) {
    try {
      port_name = midi_in->getPortName(i);
      std::cout << " - Input Port #" << i+1 << ": " << port_name << std::endl;
    } catch(RtMidiError& error) {
      error.printMessage();
      return 1;
    }
  }

  std::cout << "Which MIDI device to read from? ";
  unsigned int port;
  std::cin >> port;

  if(port > num_ports) {
    std::cerr << "Invalid MIDI device selected" << std::endl;
    return 1;
  }
  midi_in->openPort(port-1);

  // Don't ignore sysex, timing, or active sensing messages.
  midi_in->ignoreTypes( false, false, false );

  if(FLAGS_forward) {
    try {
      midi_out = new RtMidiOut();
    } catch (RtMidiError& error) {
      error.printMessage();
      return 1;
    }

    num_ports = midi_out->getPortCount();
    std::cout << num_ports << " MIDI output sources available" << std::endl;
    for(unsigned int i = 0; i < num_ports; ++i) {
      try {
        port_name = midi_out->getPortName(i);
        std::cout << " - Output Port #" << i+1 << ": " << port_name << std::endl;
      } catch(RtMidiError& error) {
        error.printMessage();
        return 1;
      }
    }

    std::cout << "Which MIDI device to output to? ";
    std::cin >> port;

    if(port > num_ports) {
      std::cerr << "Invalid MIDI device selected" << std::endl;
      return 1;
    }
    midi_out->openPort(port-1);
  }

  ScopeGuard midi_out_guard([midi_out]() {
    if(midi_out != nullptr) {
      delete midi_out;
    }
  });

  int difficulty = DIFFICULTY_EASY;
  if(FLAGS_difficulty == "med") {
    difficulty = DIFFICULTY_MED;
  } else if(FLAGS_difficulty == "hard") {
    difficulty = DIFFICULTY_HARD;
  }

  unsigned int num_asked = 0;
  unsigned int num_correct = 0;

  while(!done) {
    int random_note = get_random_note(difficulty);
    std::wstring note_name = get_note_name(random_note, FLAGS_notation);
    std::wcout.sync_with_stdio(false);
    std::wcout.imbue(std::locale("en_US.utf8"));
    std::wcout << L"Give me a " << note_name << L"... " << std::flush;

    // Discard pending unprocessed input
    std::vector<uint8_t> message;
    midi_in->getMessage(&message);

    int read_note = 0;
    while(!read_note && !done) {
      midi_in->getMessage(&message);
      unsigned int num_bytes = message.size();
      if (num_bytes) {
        if(midi_out != nullptr) {
          midi_out->sendMessage(&message);
        }

        // We're only interested on Channel 1 Note On, which is 3 bytes and status byte is 144
        if(num_bytes == 3) {
          if(message[0] == (CHANNEL_1_NOTE_ON + (FLAGS_channel - 1)) && message[2] != 0) {
            // Byte 2 is the key pressed, byte 3 is the velocity, we only
            // check this to see if it's 0, as some midi instruments send that
            // instead of the dedicated note off message
            read_note = message[1];
          }
        }
      }
    }

    if(!done) {
      // Only increment asked counted here in order to prevent counting the last one as failure
      ++num_asked;

      if(read_note == random_note) {
        std::cout << "👍" << std::endl;
        ++num_correct;
      } else {
        std::cout << "🔥" << std::endl;
      }
    }
  }

  std::cout << std::fixed;
  std::cout << std::setprecision(2);
  std::cout << std::endl << std::endl << "Accuracy rate: " << static_cast<double>(100 * num_correct) / num_asked << "%" << std::endl;

  return 0;
}
