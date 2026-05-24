#pragma once

#include <Epub.h>
#include <HalStorage.h>
#include <Logging.h>
#include <Serialization.h>

namespace EpubReaderUtils {

// Persists reader progress for an EPUB to its cache directory. Returns true on success.
inline bool saveProgress(Epub& epub, int spineIndex, int pageNumber, int pageCount) {
  if (spineIndex < 0 || spineIndex > 0xFFFF || pageNumber < 0 || pageNumber > 0xFFFF || pageCount < 0 ||
      pageCount > 0xFFFF) {
    LOG_ERR("ERS", "Progress values out of range: spine=%d page=%d count=%d", spineIndex, pageNumber, pageCount);
    return false;
  }
  FsFile f;
  if (!Storage.openFileForWrite("ERS", epub.getCachePath() + "/progress.bin", f)) {
    LOG_ERR("ERS", "Could not open progress file for write!");
    return false;
  }
  uint8_t data[6];
  data[0] = spineIndex & 0xFF;
  data[1] = (spineIndex >> 8) & 0xFF;
  data[2] = pageNumber & 0xFF;
  data[3] = (pageNumber >> 8) & 0xFF;
  data[4] = pageCount & 0xFF;
  data[5] = (pageCount >> 8) & 0xFF;
  const size_t written = f.write(data, sizeof(data));
  if (written != sizeof(data)) {
    LOG_ERR("ERS", "Short write saving progress: %u/%u bytes", (unsigned)written, (unsigned)sizeof(data));
    return false;
  }
  LOG_DBG("ERS", "Progress saved: spine=%d page=%d", spineIndex, pageNumber);
  return true;
}

// Loads the page index for an EPUB from its cache directory.
// Validates the stored settings fingerprint against current rendering parameters.
// Returns true on success; pageCounts[] is populated with one entry per spine item (0 = unknown).
inline bool loadPageIndex(Epub& epub, uint16_t* pageCounts, uint16_t spineCount, int fontId,
                          float lineCompression, bool extraParagraphSpacing, uint8_t paragraphAlignment,
                          uint16_t viewportWidth, uint16_t viewportHeight, bool hyphenationEnabled,
                          bool embeddedStyle, uint8_t imageRendering, bool focusReadingEnabled) {
  FsFile f;
  if (!Storage.openFileForRead("ERS", epub.getCachePath() + "/page_index.bin", f)) return false;

  constexpr uint8_t PAGE_INDEX_VERSION = 1;
  uint8_t version;
  serialization::readPod(f, version);
  if (version != PAGE_INDEX_VERSION) return false;

  int fileFontId;
  float fileLineCompression;
  bool fileExtraParagraphSpacing;
  uint8_t fileParagraphAlignment;
  uint16_t fileViewportWidth, fileViewportHeight;
  bool fileHyphenationEnabled;
  bool fileEmbeddedStyle;
  uint8_t fileImageRendering;
  bool fileFocusReadingEnabled;
  uint16_t fileSpineCount;
  serialization::readPod(f, fileFontId);
  serialization::readPod(f, fileLineCompression);
  serialization::readPod(f, fileExtraParagraphSpacing);
  serialization::readPod(f, fileParagraphAlignment);
  serialization::readPod(f, fileViewportWidth);
  serialization::readPod(f, fileViewportHeight);
  serialization::readPod(f, fileHyphenationEnabled);
  serialization::readPod(f, fileEmbeddedStyle);
  serialization::readPod(f, fileImageRendering);
  serialization::readPod(f, fileFocusReadingEnabled);
  serialization::readPod(f, fileSpineCount);

  if (fileFontId != fontId || fileLineCompression != lineCompression ||
      fileExtraParagraphSpacing != extraParagraphSpacing || fileParagraphAlignment != paragraphAlignment ||
      fileViewportWidth != viewportWidth || fileViewportHeight != viewportHeight ||
      fileHyphenationEnabled != hyphenationEnabled || fileEmbeddedStyle != embeddedStyle ||
      fileImageRendering != imageRendering || fileFocusReadingEnabled != focusReadingEnabled ||
      fileSpineCount != spineCount) {
    LOG_DBG("ERS", "Page index settings mismatch, will rebuild");
    return false;
  }

  const size_t dataBytes = spineCount * sizeof(uint16_t);
  if (f.read(pageCounts, dataBytes) != static_cast<int>(dataBytes)) {
    LOG_ERR("ERS", "Short read loading page index");
    return false;
  }

  LOG_DBG("ERS", "Page index loaded: %u sections", (unsigned)spineCount);
  return true;
}

// Saves the page index for an EPUB to its cache directory.
// Writes the settings fingerprint so a future load can detect staleness.
inline bool savePageIndex(Epub& epub, const uint16_t* pageCounts, uint16_t spineCount, int fontId,
                          float lineCompression, bool extraParagraphSpacing, uint8_t paragraphAlignment,
                          uint16_t viewportWidth, uint16_t viewportHeight, bool hyphenationEnabled,
                          bool embeddedStyle, uint8_t imageRendering, bool focusReadingEnabled) {
  FsFile f;
  if (!Storage.openFileForWrite("ERS", epub.getCachePath() + "/page_index.bin", f)) {
    LOG_ERR("ERS", "Could not open page index for write");
    return false;
  }

  constexpr uint8_t PAGE_INDEX_VERSION = 1;
  serialization::writePod(f, PAGE_INDEX_VERSION);
  serialization::writePod(f, fontId);
  serialization::writePod(f, lineCompression);
  serialization::writePod(f, extraParagraphSpacing);
  serialization::writePod(f, paragraphAlignment);
  serialization::writePod(f, viewportWidth);
  serialization::writePod(f, viewportHeight);
  serialization::writePod(f, hyphenationEnabled);
  serialization::writePod(f, embeddedStyle);
  serialization::writePod(f, imageRendering);
  serialization::writePod(f, focusReadingEnabled);
  serialization::writePod(f, spineCount);

  const size_t dataBytes = spineCount * sizeof(uint16_t);
  if (f.write(pageCounts, dataBytes) != dataBytes) {
    LOG_ERR("ERS", "Short write saving page index");
    return false;
  }

  LOG_DBG("ERS", "Page index saved: %u sections", (unsigned)spineCount);
  return true;
}

}  // namespace EpubReaderUtils
