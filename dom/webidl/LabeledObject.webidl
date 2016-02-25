dictionary CILabel {
  Label? confidentiality;
  Label? integrity;
};

// change DomString to object?
[Constructor(DOMString blob, CILabel labels)]
interface LabeledObject {

  // Blob privacy and trust labels
  [Pure] readonly attribute Label confidentiality;
  [Pure] readonly attribute Label integrity;

  // Underlying labeled value
  [GetterThrows] readonly attribute DOMString protectedObject;

  /*
    TODO: The spec does not state that labels should be optional, but it seems like they must be? In this case it does not matter.
    WebIDL.WebIDLError: error: Dictionary argument or union argument containing a dictionary not followed by a required argument must be optional
   */
  [Throws] LabeledObject clone(optional CILabel labels);
};