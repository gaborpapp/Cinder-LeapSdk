/*
* 
* Copyright (c) 2012, Ban the Rewind
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or 
* without modification, are permitted provided that the following 
* conditions are met:
* 
* Redistributions of source code must retain the above copyright 
* notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright 
* notice, this list of conditions and the following disclaimer in 
* the documentation and/or other materials provided with the 
* distribution.
* 
* Neither the name of the Ban the Rewind nor the names of its 
* contributors may be used to endorse or promote products 
* derived from this software without specific prior written 
* permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
* COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
*/

#include "Leap.h"
#include "boost/signals2.hpp"
#include "cinder/Thread.h"
#include "cinder/Vector.h"

namespace LeapSdk {

typedef std::shared_ptr<class Device>			DeviceRef;
typedef std::map<int32_t, class Hand>			HandMap;
typedef std::map<int32_t, class Finger>			FingerMap;
class Listener;

//////////////////////////////////////////////////////////////////////////////////////////////

class Finger
{
public:
	//! Returns normalized vector of finger pointing direction.
	const ci::Vec3f&		getDirection() const;
	//! Returns length of finger in millimeters.
	float					getLength() const;
	//! Returns position vector of finger in millimeters.
	const ci::Vec3f&		getPosition() const;
	//! Returns velocity vector of finger in millimeters.
	const ci::Vec3f&		getVelocity() const;
	//! Returns width of finger in millimeters.
	float					getWidth() const;
	
	//! Returns true if finger is holding a tool.
	bool					isTool() const;
private:
	Finger( const ci::Vec3f& position = ci::Vec3f::zero(), const ci::Vec3f& direction = ci::Vec3f::zero(),
		   const ci::Vec3f& velocity = ci::Vec3f::zero(), float length = 0.0f, float width = 0.0f,
		   bool isTool = false );

	ci::Vec3f				mDirection;
	bool					mIsTool;
	float					mLength;
	ci::Vec3f				mPosition;
	ci::Vec3f				mVelocity;
	float					mWidth;

	friend FingerMap::mapped_type& FingerMap::operator[]( FingerMap::key_type&& );
	friend class			Listener;
};

//////////////////////////////////////////////////////////////////////////////////////////////

class Hand 
{
public:
	~Hand();

	//! Returns position vector of hand ball in millimeters.
	const ci::Vec3f&		getBallPosition() const;
	//! Returns radius of hand ball in millimeters.
	float					getBallRadius() const;
	//! Returns normalized vector of palm face direction.
	const ci::Vec3f&		getDirection() const;
	//! Returns map of finger object.
	const FingerMap&		getFingers() const;
	//! Returns hand normal vector.
	const ci::Vec3f&		getNormal() const;
	//! Returns position vector of hand in millimeters.
	const ci::Vec3f&		getPosition() const;
	//! Returns velocity vector of hand in millimeters.
	const ci::Vec3f&		getVelocity() const;
private:
	Hand( const FingerMap& fingerMap = FingerMap(), const ci::Vec3f& position = ci::Vec3f::zero(),
		 const ci::Vec3f& direction = ci::Vec3f::zero(), const ci::Vec3f& velocity = ci::Vec3f::zero(),
		 const ci::Vec3f& normal = ci::Vec3f::zero(), const ci::Vec3f& ballPosition = ci::Vec3f::zero(),
		 float ballRadius = 0.0f );

	ci::Vec3f				mBallPosition;
	float					mBallRadius;
	ci::Vec3f				mDirection;
	FingerMap				mFingers;
	ci::Vec3f				mNormal;
	ci::Vec3f				mPosition;
	ci::Vec3f				mVelocity;

	friend HandMap::mapped_type& HandMap::operator[]( HandMap::key_type&& );
	friend class			Listener;
};

//////////////////////////////////////////////////////////////////////////////////////////////

class Frame
{
public:
	~Frame();

	//! Returns map of hands.
	const HandMap&			getHands() const;
	// Returns frame ID.
	int64_t					getId() const;
	// Return time stamp.
	int64_t					getTimestamp() const;
private:
	Frame( const HandMap& handMap = HandMap(), int64_t id = 0, int64_t timestamp = 0 );
	HandMap					mHands;
	int64_t					mId;
	int64_t					mTimestamp;

	friend class			Listener;
};

//////////////////////////////////////////////////////////////////////////////////////////////

class Listener : public Leap::Listener
{
protected:
	Listener( std::mutex *mutex );
    virtual void			onConnect( const Leap::Controller& controller );
    virtual void			onDisconnect( const Leap::Controller& controller );
    virtual void			onFrame( const Leap::Controller& controller );
	virtual void			onInit( const Leap::Controller& controller );

	volatile bool			mConnected;
	volatile bool			mInitialized;
	std::mutex				*mMutex;
	volatile bool			mNewFrame;

	Frame					mFrame;

	friend class			Device;
};

//////////////////////////////////////////////////////////////////////////////////////////////

class Device
{
public:
	//! Creates and returns device instance.
	static DeviceRef		create();
	~Device();
	
	//! Must be called to trigger frame events.
	void					update();

	//! Returns true if the device is connected.
	bool					isConnected() const;
	//! Returns true if LEAP application is initialized.
	bool					isInitialized() const;

	/*! Adds frame event callback. \a callback has the signature \a void(Frame). 
		\a callbackObject is the instance receiving the event. Returns callback ID.*/
	template<typename T, typename Y> 
	inline uint32_t			addCallback( T callback, Y *callbackObject )
	{
		uint32_t id = mCallbacks.empty() ? 0 : mCallbacks.rbegin()->first + 1;
		mCallbacks.insert( std::make_pair( id, CallbackRef( new Callback( mSignal.connect( std::bind( callback, callbackObject, std::placeholders::_1 ) ) ) ) ) );
		return id;
	}
	//! Remove callback by ID.
	void					removeCallback( uint32_t id );
private:
	Device();

	typedef boost::signals2::connection		Callback;
	typedef std::shared_ptr<Callback>		CallbackRef;
	typedef std::map<uint32_t, CallbackRef>	CallbackList;

	CallbackList							mCallbacks;
	boost::signals2::signal<void ( Frame )>	mSignal;

	Leap::Controller*		mController;
	Listener*				mListener;
	std::mutex				mMutex;
};

}