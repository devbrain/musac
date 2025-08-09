#include <doctest/doctest.h>
#include <musac/sdk/mml_parser.hh>
#include <cmath>

TEST_CASE("MML Parser - Basic Notes") {
    musac::mml_parser parser;
    
    SUBCASE("Single note") {
        auto events = parser.parse("C");
        REQUIRE(events.size() == 1);
        CHECK(events[0].event_type == musac::mml_event::type::note);
        CHECK(doctest::Approx(events[0].frequency_hz) == 261.63f);
        CHECK(events[0].duration.count() == 500); // Default tempo 120, quarter note
    }
    
    SUBCASE("Note with length") {
        auto events = parser.parse("C8");
        REQUIRE(events.size() == 1);
        CHECK(events[0].duration.count() == 250); // Eighth note
    }
    
    SUBCASE("Note with sharp") {
        auto events = parser.parse("C#");
        REQUIRE(events.size() == 1);
        CHECK(doctest::Approx(events[0].frequency_hz) == 277.18f);
    }
    
    SUBCASE("Note with flat") {
        auto events = parser.parse("D-");
        REQUIRE(events.size() == 1);
        CHECK(doctest::Approx(events[0].frequency_hz) == 277.18f); // Same as C#
    }
    
    SUBCASE("All notes in octave") {
        auto events = parser.parse("C D E F G A B");
        REQUIRE(events.size() == 7);
        CHECK(doctest::Approx(events[0].frequency_hz) == 261.63f); // C
        CHECK(doctest::Approx(events[1].frequency_hz) == 293.66f); // D
        CHECK(doctest::Approx(events[2].frequency_hz) == 329.63f); // E
        CHECK(doctest::Approx(events[3].frequency_hz) == 349.23f); // F
        CHECK(doctest::Approx(events[4].frequency_hz) == 392.00f); // G
        CHECK(doctest::Approx(events[5].frequency_hz) == 440.00f); // A
        CHECK(doctest::Approx(events[6].frequency_hz) == 493.88f); // B
    }
}

TEST_CASE("MML Parser - Rests") {
    musac::mml_parser parser;
    
    SUBCASE("Rest with R") {
        auto events = parser.parse("R");
        REQUIRE(events.size() == 1);
        CHECK(events[0].event_type == musac::mml_event::type::rest);
        CHECK(events[0].frequency_hz == 0.0f);
        CHECK(events[0].duration.count() == 500); // Default quarter rest
    }
    
    SUBCASE("Rest with P") {
        auto events = parser.parse("P8");
        REQUIRE(events.size() == 1);
        CHECK(events[0].event_type == musac::mml_event::type::rest);
        CHECK(events[0].duration.count() == 250); // Eighth rest
    }
}

TEST_CASE("MML Parser - Octaves") {
    musac::mml_parser parser;
    
    SUBCASE("Octave command") {
        auto events = parser.parse("O3 C O5 C");
        REQUIRE(events.size() == 4); // 2 octave changes + 2 notes
        CHECK(events[0].event_type == musac::mml_event::type::octave_change);
        CHECK(events[0].value == 3);
        CHECK(doctest::Approx(events[1].frequency_hz) == 130.815f); // C3
        CHECK(doctest::Approx(events[3].frequency_hz) == 523.26f); // C5
    }
    
    SUBCASE("Octave up/down") {
        auto events = parser.parse("C >C <C");
        REQUIRE(events.size() == 5); // 3 notes + 2 octave changes
        CHECK(doctest::Approx(events[0].frequency_hz) == 261.63f); // C4
        CHECK(doctest::Approx(events[2].frequency_hz) == 523.26f); // C5
        CHECK(doctest::Approx(events[4].frequency_hz) == 261.63f); // C4
    }
}

TEST_CASE("MML Parser - Tempo") {
    musac::mml_parser parser;
    
    SUBCASE("Tempo change") {
        auto events = parser.parse("T60 C T240 C");
        REQUIRE(events.size() == 4); // 2 tempo changes + 2 notes
        CHECK(events[0].event_type == musac::mml_event::type::tempo_change);
        CHECK(events[0].value == 60);
        CHECK(events[1].duration.count() == 1000); // Quarter note at 60 BPM
        CHECK(events[3].duration.count() == 250); // Quarter note at 240 BPM
    }
}

TEST_CASE("MML Parser - Length") {
    musac::mml_parser parser;
    
    SUBCASE("Default length change") {
        auto events = parser.parse("L8 C D E");
        REQUIRE(events.size() == 3);
        CHECK(events[0].duration.count() == 250); // Eighth note
        CHECK(events[1].duration.count() == 250);
        CHECK(events[2].duration.count() == 250);
    }
    
    SUBCASE("Mixed lengths") {
        auto events = parser.parse("L4 C C8 C16 C32");
        REQUIRE(events.size() == 4);
        CHECK(events[0].duration.count() == 500); // Quarter
        CHECK(events[1].duration.count() == 250); // Eighth
        CHECK(events[2].duration.count() == 125); // Sixteenth
        CHECK(events[3].duration.count() == 62); // Thirty-second
    }
}

TEST_CASE("MML Parser - Dotted Notes") {
    musac::mml_parser parser;
    
    SUBCASE("Single dot") {
        auto events = parser.parse("C4.");
        REQUIRE(events.size() == 1);
        CHECK(events[0].duration.count() == 750); // 500 * 1.5
    }
    
    SUBCASE("Double dot") {
        auto events = parser.parse("C4..");
        REQUIRE(events.size() == 1);
        CHECK(events[0].duration.count() == 750); // Limited to single dot effect
    }
}

TEST_CASE("MML Parser - Articulation") {
    musac::mml_parser parser;
    
    SUBCASE("Staccato") {
        auto events = parser.parse("MS C");
        auto tones = musac::mml_to_tones::convert(events);
        REQUIRE(tones.size() == 2); // Note + gap
        CHECK(tones[0].duration.count() == 375); // 500 * 0.75
        CHECK(tones[1].frequency_hz == 0.0f); // Gap
        CHECK(tones[1].duration.count() == 125); // 500 * 0.25 gap
    }
    
    SUBCASE("Legato") {
        auto events = parser.parse("ML C");
        auto tones = musac::mml_to_tones::convert(events);
        REQUIRE(tones.size() == 1);
        CHECK(tones[0].duration.count() == 500); // Full length
    }
    
    SUBCASE("Normal") {
        auto events = parser.parse("MN C");
        auto tones = musac::mml_to_tones::convert(events);
        REQUIRE(tones.size() == 2); // Note + gap (normal has a small gap)
        CHECK(tones[0].duration.count() == 437); // 500 * 0.875
        CHECK(tones[1].frequency_hz == 0.0f); // Gap
        CHECK(tones[1].duration.count() == 62); // 500 * 0.125 gap (with rounding)
    }
}

TEST_CASE("MML Parser - Error Handling") {
    musac::mml_parser parser;
    
    SUBCASE("Invalid note in non-strict mode") {
        parser.set_strict_mode(false);
        auto events = parser.parse("C Z D");
        CHECK(events.size() == 2); // Z is skipped
        CHECK(parser.get_warnings().size() == 1);
        CHECK(parser.get_warnings()[0].find("Unknown command") != std::string::npos);
    }
    
    SUBCASE("Invalid note in strict mode") {
        parser.set_strict_mode(true);
        CHECK_THROWS_AS(parser.parse("C Z D"), musac::mml_error);
    }
    
    SUBCASE("Out of range tempo") {
        parser.set_strict_mode(false);
        auto events = parser.parse("T300 C"); // Max is 255
        CHECK(parser.get_warnings().size() == 1);
        CHECK(parser.get_warnings()[0].find("out of range") != std::string::npos);
    }
    
    SUBCASE("Out of range octave") {
        parser.set_strict_mode(false);
        auto events = parser.parse("O8 C"); // Max is 6
        CHECK(parser.get_warnings().size() == 1);
    }
}

TEST_CASE("MML Parser - Complex Examples") {
    musac::mml_parser parser;
    
    SUBCASE("Mary Had a Little Lamb") {
        auto events = parser.parse("T120 L4 E D C D E E E2 D D D2 E G G2");
        CHECK(events.size() > 10);
        CHECK(parser.get_warnings().empty());
    }
    
    SUBCASE("Scale with octave changes") {
        auto events = parser.parse("T120 L4 C D E F G A B >C");
        CHECK(events.size() == 10); // 1 tempo + 8 notes + 1 octave change
        CHECK(parser.get_warnings().empty());
    }
    
    SUBCASE("Whitespace handling") {
        auto events1 = parser.parse("C D E");
        auto events2 = parser.parse("  C  D  E  ");
        auto events3 = parser.parse("C\nD\tE");
        CHECK(events1.size() == events2.size());
        CHECK(events1.size() == events3.size());
    }
}

TEST_CASE("MML to Tones Conversion") {
    musac::mml_parser parser;
    
    SUBCASE("Basic conversion") {
        auto events = parser.parse("C D E R F");
        auto tones = musac::mml_to_tones::convert(events);
        // With normal articulation: 3 notes with gaps + 1 rest + 1 note with gap = 9 tones
        REQUIRE(tones.size() == 9);
        CHECK(tones[0].frequency_hz > 0.0f); // C
        CHECK(tones[1].frequency_hz == 0.0f); // Gap
        CHECK(tones[2].frequency_hz > 0.0f); // D
        CHECK(tones[3].frequency_hz == 0.0f); // Gap
        CHECK(tones[4].frequency_hz > 0.0f); // E
        CHECK(tones[5].frequency_hz == 0.0f); // Gap
        CHECK(tones[6].frequency_hz == 0.0f); // Rest
        CHECK(tones[7].frequency_hz > 0.0f); // F
        CHECK(tones[8].frequency_hz == 0.0f); // Gap
    }
    
    SUBCASE("With articulation") {
        auto events = parser.parse("MS C D ML E F MN G A");
        auto tones = musac::mml_to_tones::convert_with_articulation(events);
        // Should have notes plus gaps for staccato/normal
        CHECK(tones.size() > 6);
    }
}