#ifndef ISHADER_H_
#define ISHADER_H_

#include "texturelib.h"
#include "ishaderlayer.h"

class IShader
{
	public:

		enum EAlphaFunc
		{
			eAlways, eEqual, eLess, eGreater, eLEqual, eGEqual
		};

		enum ECull
		{
			eCullNone, eCullBack
		};

		virtual ~IShader ()
		{
		}

		/**
		 * Increment the number of references to this object
		 */
		virtual void IncRef () = 0;

		/**
		 * Decrement the reference count
		 */
		virtual void DecRef () = 0;

		/**
		 * get/set the qtexture_t* Radiant uses to represent this shader object
		 */
		virtual qtexture_t* getTexture () const = 0;

		/**
		 * get shader name
		 */
		virtual const char* getName () const = 0;

		virtual bool IsInUse () const = 0;

		virtual void SetInUse (bool bInUse) = 0;

		virtual bool IsValid () const = 0;

		virtual void SetIsValid (bool bIsValid) = 0;

		/**
		 * get the editor flags (QER_TRANS)
		 */
		virtual int getFlags () const = 0;

		/**
		 * get the transparency value
		 */
		virtual float getTrans () const = 0;

		/**
		 * test if it's a true shader, or a default shader created to wrap around a texture
		 */
		virtual bool IsDefault () const = 0;

		/**
		 * get the alphaFunc
		 */
		virtual void getAlphaFunc (EAlphaFunc *func, float *ref) = 0;

		virtual BlendFunc getBlendFunc () const = 0;

		virtual void forEachLayer(const ShaderLayerCallback& layer) const = 0;

	    float getPolygonOffset() const;

		/**
		 * get the cull type
		 */
		virtual ECull getCull () = 0;
};

#endif /* ISHADER_H_ */
