#include "sys/video.h"
#include "print/output.h"
#include "sys/base.h"
#include "sys/input.h"
#include "sys/interrupts.h"
#include "sys/memctrl.h"
#include "tui.h"
#include "videoplayer.h"

#include "data/video.h"

int main()
{
	// start wall clock
	Irq::init();
	// set up text UI
	TUI::setup();
	TUI::fillBackground(TUI::Color::Black);
	// set waitstates for GamePak ROM
	if (!MemCtrl::setWaitCnt(MemCtrl::WaitCntFast))
	{
		if (MemCtrl::setWaitCnt(MemCtrl::WaitCntNormal))
		{
			TUI::setColor(TUI::Color::Black, TUI::Color::Yellow);
			TUI::printf(0, 9, "      Slow ROM detected");
			TUI::printf(0, 10, " Playback might not be optimal");
		}
		else
		{
			TUI::setColor(TUI::Color::Black, TUI::Color::Red);
			TUI::printf(0, 9, "    Very slow ROM detected");
			TUI::printf(0, 10, "   Expect playback problems");
		}
		TUI::setColor(TUI::Color::Black, TUI::Color::LightGray);
		TUI::printf(0, 19, "     Press A to continue");
		// wait for keypress
		Input::waitForKeysDown(Input::KeyA, true);
		TUI::fillForeground(TUI::Color::Black);
	}
	// get media info and check if we have meta data
	const auto mediaInfo = IO::Vid2h::GetInfo(reinterpret_cast<const uint32_t *>(VIDEO_DATA), VIDEO_DATA_SIZE);
	if (mediaInfo.metaDataSize > 0 && mediaInfo.metaData != nullptr)
	{
		char temp[20];
		const uint32_t maxSize = mediaInfo.metaDataSize < 19 ? mediaInfo.metaDataSize : 19;
		uint32_t i = 0;
		for (; i < maxSize; ++i)
		{
			temp[i] = mediaInfo.metaData[i];
		}
		temp[i] = '\0';
		TUI::printf(0, 0, "Meta data: %s", temp);
	}
	// print media info
	TUI::printf(0, 1, "File size: %d kB", VIDEO_DATA_SIZE / 1024);
	if (mediaInfo.contentType & IO::FileType::Video)
	{
		TUI::setColor(TUI::Color::Black, TUI::Color::Cyan);
		TUI::printf(0, 3, "Video: %dx%d @ %d bpp", mediaInfo.video.width, mediaInfo.video.height, mediaInfo.video.bitsPerPixel);
		TUI::printf(0, 4, "Frames: %d, Fps: %f", mediaInfo.video.nrOfFrames, mediaInfo.video.frameRateHz);
		TUI::printf(0, 5, "Duration: %f s", int32_t((uint64_t(mediaInfo.video.nrOfFrames) << 32) / mediaInfo.video.frameRateHz));
		TUI::printf(0, 6, "Colormap size: %d @ %d bpp", mediaInfo.video.colorMapEntries, mediaInfo.video.bitsPerColor);
		TUI::printf(0, 7, "Color map frames: %d", mediaInfo.video.nrOfColorMapFrames);
		TUI::printf(0, 8, "Red-Blue swapped: %b", mediaInfo.video.swappedRedBlue);
		TUI::printf(0, 9, "Memory needed: %d Byte", mediaInfo.video.memoryNeeded);
	}
	else
	{
		TUI::setColor(TUI::Color::Black, TUI::Color::Cyan);
		TUI::printf(0, 3, "No video data");
	}
	if (mediaInfo.contentType & IO::FileType::Audio)
	{
		TUI::setColor(TUI::Color::Black, TUI::Color::Magenta);
		TUI::printf(0, 11, "Audio: %s, Samples: %d", mediaInfo.audio.channels == 2 ? "Stereo" : "Mono", mediaInfo.audio.nrOfSamples);
		TUI::printf(0, 12, "Rate: %d Hz, Depth: %d bit", mediaInfo.audio.sampleRateHz, mediaInfo.audio.sampleBits);
		TUI::printf(0, 13, "Duration: %f s", int32_t((uint64_t(mediaInfo.audio.nrOfSamples) << 16) / mediaInfo.audio.sampleRateHz));
		TUI::printf(0, 14, "Memory needed: %d Byte", mediaInfo.audio.memoryNeeded);
	}
	else
	{
		TUI::setColor(TUI::Color::Black, TUI::Color::Magenta);
		TUI::printf(0, 11, "No audio data");
	}
	if (mediaInfo.contentType & IO::FileType::Subtitles)
	{
		TUI::setColor(TUI::Color::Black, TUI::Color::Yellow);
		TUI::printf(0, 16, "Subtitles: %d frames", mediaInfo.subtitles.nrOfFrames);
	}
	else
	{
		TUI::setColor(TUI::Color::Black, TUI::Color::Yellow);
		TUI::printf(0, 16, "No subtitles data");
	}
	TUI::setColor(TUI::Color::Black, TUI::Color::LightGray);
	TUI::printf(0, 19, "    Press A to play (again)");
	// center video on screen
	Media::SetPosition((240 - mediaInfo.video.width) / 2, (160 - mediaInfo.video.height) / 2);
	// wait for keypress
	Input::waitForKeysDown(Input::KeyA, true);
	// switch video mode to 240x160x2
	REG_DISPCNT = MODE_3 | BG2_ON;
	// set up video system and clear color
	Media::SetDisplayInfo(480, 2);
	Media::SetClearColor(0);
	// start main loop
	Media::Play(reinterpret_cast<const uint32_t *>(VIDEO_DATA), VIDEO_DATA_SIZE);
	do
	{
		Media::DecodeAndPlay();
		if (!Media::HasMoreFrames())
		{
			Media::Stop();
			Input::waitForKeysDown(Input::KeyA, true);
			Media::Play(reinterpret_cast<const uint32_t *>(VIDEO_DATA), VIDEO_DATA_SIZE);
		}
	} while (true);
	return 0;
}
