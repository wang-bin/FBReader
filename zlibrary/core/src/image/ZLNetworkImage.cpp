/*
 * Copyright (C) 2009-2010 Geometer Plus <contact@geometerplus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <cstring>

#include <ZLibrary.h>
#include <ZLStringUtil.h>
#include <ZLNetworkManager.h>
#include <ZLDir.h>
#include <ZLFile.h>
#include <ZLFileImage.h>

#include <ZLOutputStream.h>

#include "ZLNetworkImage.h"

#include "ZLImageManager.h"


class ImageDownloadListener : public ZLExecutionData::Listener {

public:
	ImageDownloadListener(const ZLImage &image);

private:
	bool finish();

private:
	const ZLImage &myImage;
};

ImageDownloadListener::ImageDownloadListener(const ZLImage &image) : myImage(image) {
}

bool ImageDownloadListener::finish() {
	// FIXME: now imageData() always returns not null -- we need better image validation
	return !ZLImageManager::Instance().imageData(myImage).isNull();
}



ZLNetworkImage::ZLNetworkImage(const std::string &mimeType, const std::string &url) : ZLSingleImage(mimeType), myURL(url), myIsSynchronized(false) {
	static const std::string directoryPath = ZLNetworkManager::CacheDirectory();

	std::string prefix;
	if (ZLStringUtil::stringStartsWith(myURL, "http://")) {
		prefix = "http://";
	} else if (ZLStringUtil::stringStartsWith(myURL, "https://")) {
		prefix = "https://";
	} else if (ZLStringUtil::stringStartsWith(myURL, "ftp://")) {
		prefix = "ftp://";
	} else {
		myIsSynchronized = true;
		return;
	}
	myFileName = myURL.substr(prefix.length());
	myFileName = ZLFile::replaceIllegalCharacters(myFileName, '_');
	myFileName = directoryPath + ZLibrary::FileNameDelimiter + myFileName;

	static shared_ptr<ZLDir> dir = ZLFile(directoryPath).directory(true);
	if (dir.isNull()) {
		myIsSynchronized = true;
		return;
	}

	ZLFile imageFile(myFileName);
	if (imageFile.exists()) {
		myIsSynchronized = true;
		myCachedImage = new ZLFileImage("image/auto", myFileName, 0);
		// TODO: проверить картинку на валидность (если не валидна, стереть файл и myIsSynchronized = false)
	}
}

shared_ptr<ZLExecutionData> ZLNetworkImage::synchronizationData() const {
	if (myIsSynchronized) {
		return 0;
	}
	myIsSynchronized = true;

	shared_ptr<ZLExecutionData> request = ZLNetworkManager::Instance().createDownloadRequest(myURL, myFileName);
	// убрать listener, передавать в request ссылку на this, пусть он ставит в ZLNetworkImage нужные флаги
	request->setListener(new ImageDownloadListener(*this));
	return request;
}

const shared_ptr<std::string> ZLNetworkImage::stringData() const {
	if (myCachedImage.isNull()) {
		ZLFile imageFile(myFileName);
		if (imageFile.exists()) {
			myCachedImage = new ZLFileImage("image/auto", myFileName, 0);
		}
	}
	return myCachedImage.isNull() ? 0 : myCachedImage->stringData();
}
