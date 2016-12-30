/*
 * Copyright (c) 2016 Sound <sound@sagaforce.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <condition_variable>

namespace Nebula
{
	template<typename T>
	class AsyncProgressData
	{
	public:
		AsyncProgressData()
		: mIsDone(false)
		, mIsReady(false)
		, mCancelRequested(false)
		, mCancelled(false)
		, mProgress(0.0f)
		, mOnDoneCallback(nullptr)
		, mOnCancelledCallback(nullptr)
		, mProgressCallback(nullptr)
		, mOnErrorCallback(nullptr) { }

		void setReady(const T& result)
		{
			std::lock_guard<std::mutex> lock(mMutex);
			mResult = result;
			mIsReady = true;
			mIsDone = true;
			mDoneCondition.notify_all();
		}
		
		void setError(const std::string& errorMessage)
		{
			std::lock_guard<std::mutex> lock(mMutex);
			mErrorMessage = errorMessage;
			mIsDone = true;
			mDoneCondition.notify_all();
			
			if(mOnErrorCallback) {
				mOnErrorCallback(errorMessage);
			}
		}

		bool cancelRequested() { return mCancelRequested; }

		void requestCancel()
		{
			mCancelRequested = true;
		}
		
		void setCancelled()
		{
			{
				std::lock_guard<std::mutex> lock(mMutex);
				if(mOnCancelledCallback) {
					mOnCancelledCallback();
				}
				mCancelled = true;
			}
			setError("User cancelled.");
		}

		void setProgress(float progress)
		{
			std::lock_guard<std::mutex> lock(mMutex);
			if(progress > mProgress) {
				mProgress = progress;
				if(mProgressCallback) mProgressCallback(mProgress);
			}
		}
		
		bool isDone() const {
			std::lock_guard<std::mutex> lock(mMutex);
			return mIsDone;
		}

		bool isReady() const {
			std::lock_guard<std::mutex> lock(mMutex);
			return mIsReady;
		}
		
		float progress() const {
			std::lock_guard<std::mutex> lock(mMutex);
			return mProgress;
		}
		
		bool hasError() const {
			std::lock_guard<std::mutex> lock(mMutex);
			return !mErrorMessage.empty();
		}
		
		std::string errorMessage() const {
			std::lock_guard<std::mutex> lock(mMutex);
			return mErrorMessage;
		}

		const T& result() const {
			std::lock_guard<std::mutex> lock(mMutex);
			return mResult;
		}
		
		void wait() const {
			std::unique_lock<std::mutex> lock(mMutex);
			while(!mIsDone) {
				mDoneCondition.wait(lock);
			}
		}
		
		typedef std::function<void (const T&)> OnDoneCallback;
		typedef std::function<void ()> OnCancelledCallback;
		typedef std::function<void (float)> OnProgressCallback;
		typedef std::function<void (const std::string&)> OnErrorCallback;
		
		void registerOnDoneCallback(OnDoneCallback callback)
		{
			std::unique_lock<std::mutex> lock(mMutex);
			if(mIsReady) {
				callback(mResult);
			}
			mOnDoneCallback = callback;
		}
		
		void registerOnCancelledCallback(OnCancelledCallback callback)
		{
			std::unique_lock<std::mutex> lock(mMutex);
			if(mCancelled) {
				callback();
			}
			mOnCancelledCallback = callback;
		}
		
		void registerOnProgressCallback(OnProgressCallback callback)
		{
			std::unique_lock<std::mutex> lock(mMutex);
			mProgressCallback = callback;
			mProgressCallback(mProgress);
		}
		
		void registerOnErrorCallback(OnErrorCallback callback)
		{
			std::unique_lock<std::mutex> lock(mMutex);
			if(!mErrorMessage.empty()) {
				callback(mErrorMessage);
			}
			mOnErrorCallback = callback;
		}

	private:
		bool mIsDone;
		bool mIsReady;
		bool mCancelRequested;
		bool mCancelled;
		T mResult;
		float mProgress;
		std::string mErrorMessage;
		
		OnDoneCallback mOnDoneCallback;
		OnCancelledCallback mOnCancelledCallback;
		OnProgressCallback mProgressCallback;
		OnErrorCallback mOnErrorCallback;

		mutable std::mutex mMutex;
		mutable std::condition_variable mDoneCondition;
	};
	
	template<typename T>
	class AsyncProgress
	{
	public:
		AsyncProgress(std::shared_ptr<AsyncProgressData<T>> data) : mData(data) { }
		
		bool isDone() const { return mData->isDone(); }
		bool isReady() const { return mData->isReady(); }
		float progress() const { return mData->progress(); }
		bool hasError() const { return mData->hasError(); }
		const T& result() const { return mData->result(); }
		std::string errorMessage() const { return mData->errorMessage(); }
		
		void cancel() { mData->requestCancel(); }
		void wait() const { mData->wait(); }

		template<typename U>
		AsyncProgress onDone(U func) {
			mData->registerOnDoneCallback(func);
			return *this;
		}
		
		template<typename U>
		AsyncProgress onCancelled(U func) {
			mData->registerOnCancelledCallback(func);
			return *this;
		}
		
		template<typename U>
		AsyncProgress onProgress(U func) {
			mData->registerOnProgressCallback(func);
			return *this;
		}
		
		template<typename U>
		AsyncProgress onError(U func) {
			mData->registerOnErrorCallback(func);
			return *this;
		}
		
	private:
		std::shared_ptr<AsyncProgressData<T>> mData;
	};
}
