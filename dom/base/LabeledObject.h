#pragma once

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/Label.h"
#include "mozilla/dom/LabeledObjectBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"

struct JSContext;

namespace mozilla {
namespace dom {

class LabeledObject final : public nsISupports
                            , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(LabeledObject)

protected:
  ~LabeledObject() { }

public:
  LabeledObject(const nsAString& blob, Label& confidentiality, Label& integrity);


  LabeledObject* GetParentObject() const
  {
    return nullptr; //TODO: return something sensible here
  }

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override
  {
    return LabeledObjectBinding::Wrap(aCx, this, aGivenProto);
  }

  static already_AddRefed<LabeledObject> Constructor(const GlobalObject& global,
                                                   JSContext* cx,
                                                   const nsAString& blob,
                                                   const CILabel& labels,
                                                   ErrorResult& aRv);

  already_AddRefed<Label> Confidentiality() const;
  already_AddRefed<Label> Integrity() const;

  void GetProtectedObject(JSContext* cx, nsString& aRetVal, ErrorResult& aRv) const;

  already_AddRefed<LabeledObject> Clone(const CILabel& labels, ErrorResult &aRv) const;


public: // Internal
  // bool WriteStructuredClone(JSContext* cx, JSStructuredCloneWriter* writer);
  // static JSObject* ReadStructuredClone(JSContext* cx, JSStructuredCloneReader* reader, uint32_t data);

private:
  nsString GetProtectedObject() { return mBlob; };
  RefPtr<Label> mConfidentiality;
  RefPtr<Label> mIntegrity;
  nsString  mBlob;
};

} // namespace dom
} // namespace mozilla
