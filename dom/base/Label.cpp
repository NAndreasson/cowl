/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/Label.h"
#include "mozilla/dom/LabelBinding.h"
#include "nsContentUtils.h"
#include "mozilla/dom/Privilege.h"
#include "mozilla/dom/BindingUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(Label)
//NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(Label, mRoles)
NS_IMPL_CYCLE_COLLECTING_ADDREF(Label)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Label)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Label)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Label::Label()
{
  // construct an empty set
  /* DisjunctionSet emptyDset = DisjunctionSetUtils::ConstructDset(); */
  /* mRoles.AppendElement(emptyDset); */
}

Label::Label(DisjunctionSet &dset, ErrorResult &aRv)
{
  _And(dset,aRv);
}

Label::Label(const nsAString& principal, ErrorResult &aRv)
{
  COWLPrincipal newPrincipal = COWLPrincipalUtils::ConstructPrincipal(principal, aRv);

  if (aRv.Failed()) return;

  DisjunctionSet dset = DisjunctionSetUtils::ConstructDset(newPrincipal);

  _And(dset, aRv);
}

Label::Label(nsIPrincipal *principal, ErrorResult &aRv)
{
  COWLPrincipal newPrincipal = COWLPrincipalUtils::ConstructPrincipal(principal, aRv);
  if (aRv.Failed()) return;

  DisjunctionSet dset = DisjunctionSetUtils::ConstructDset(newPrincipal);
  _And(dset, aRv);
}

Label::~Label()
{
}

JSObject*
Label::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return LabelBinding::Wrap(aCx, this, aGivenProto);
}

Label*
Label::GetParentObject() const
{
  return nullptr; //TODO: return something sensible here
}

already_AddRefed<Label>
Label::Constructor(const GlobalObject& global, ErrorResult& aRv)
{
  RefPtr<Label> label = new Label();
  if (!label) {
    aRv = NS_ERROR_OUT_OF_MEMORY;
    return nullptr;
  }
  return label.forget();
}

already_AddRefed<Label>
Label::Constructor(const GlobalObject& global, const nsAString& principal,
                   ErrorResult& aRv) {
  /* RefPtr<Label> lbl = COWLParser::parsePrincipalExpression(principal); */
  /* nsAutoString meck; */
  /* lbl->Stringify(meck); */
  /* printf("Lbl: %s\n", ToNewUTF8String(meck)); */

  COWLPrincipal newPrincipal = COWLPrincipalUtils::ConstructPrincipal(principal, aRv);
  DisjunctionSet newDSet = DisjunctionSetUtils::ConstructDset(newPrincipal);

  RefPtr<Label> label = new Label(newDSet, aRv);
  if (aRv.Failed())
    return nullptr;
  return label.forget();
}


bool
Label::Subsumes(const mozilla::dom::Label& other, const Optional<NonNull<Privilege>>& priv)
{
  if (!priv.WasPassed()) return Subsumes(other);

  Privilege& privilege = priv.Value();
  Label* privilegeLabel = privilege.DirectLabel();

  return Subsumes(*static_cast<const mozilla::dom::Label*>(privilegeLabel), other);
}

bool
Label::Subsumes(const mozilla::dom::Label& other)
{
  // Break out early if the other and this are the same.
  if (&other == this)
    return true;

  DisjunctionSetArray *otherRoles =
    const_cast<mozilla::dom::Label&>(other).GetDirectRoles();

  /* There are more roles in the other formula, this label cannot
   * imply (subsume) it. */
  if (otherRoles->Length() >  mRoles.Length())
    return false;

  RoleSubsumeComparator cmp;
  for (unsigned i = 0; i<otherRoles->Length(); ++i) {
    /* The other label has a role (the ith role) that no role in this
     * label subsumes. */
    if (!mRoles.Contains(otherRoles->ElementAt(i),cmp))
      return false;
  }

  return true;
}

already_AddRefed<Label>
Label::And(const nsAString& principal, ErrorResult& aRv)
{
  COWLPrincipal newPrincipal = COWLPrincipalUtils::ConstructPrincipal(principal, aRv);
  DisjunctionSet newDSet = DisjunctionSetUtils::ConstructDset(newPrincipal);

  RefPtr<Label> _this = Clone(aRv);
  if (aRv.Failed())
    return nullptr;

  _this->_And(newDSet, aRv);
  if (aRv.Failed())
    return nullptr;

  return _this.forget();
}

already_AddRefed<Label>
Label::And(DisjunctionSet& role, ErrorResult& aRv)
{
  RefPtr<Label> _this = Clone(aRv);
  if (aRv.Failed())
    return nullptr;

  _this->_And(role, aRv);
  if (aRv.Failed())
    return nullptr;

  return _this.forget();
}

already_AddRefed<Label>
Label::And(mozilla::dom::Label& other, ErrorResult& aRv)
{
  RefPtr<Label> _this = Clone(aRv);
  if (aRv.Failed())
    return nullptr;

  _this->_And(other, aRv);
  if (aRv.Failed())
    return nullptr;

  return _this.forget();
}

already_AddRefed<Label>
Label::Or(const nsAString& principal, ErrorResult& aRv)
{
  RefPtr<Label> _this = Clone(aRv);
  if (aRv.Failed())
    return nullptr;

  COWLPrincipal newPrincipal = COWLPrincipalUtils::ConstructPrincipal(principal, aRv);
  DisjunctionSet newDSet = DisjunctionSetUtils::ConstructDset(newPrincipal);

  _this->_Or(newDSet, aRv);
  if (aRv.Failed())
    return nullptr;

  return _this.forget();
}


already_AddRefed<Label>
Label::Or(DisjunctionSet& role, ErrorResult& aRv)
{
  RefPtr<Label> _this = Clone(aRv);
  if (aRv.Failed())
    return nullptr;

  _this->_Or(role, aRv);
  if (aRv.Failed())
    return nullptr;

  return _this.forget();
}

already_AddRefed<Label>
Label::Or(mozilla::dom::Label& other, ErrorResult& aRv)
{
  RefPtr<Label> _this = Clone(aRv);
  if (aRv.Failed())
    return nullptr;

  _this->_Or(other, aRv);
  if (aRv.Failed())
    return nullptr;

  return _this.forget();
}

bool
Label::IsEmpty() const
{
  return !mRoles.Length();
}

already_AddRefed<Label>
Label::Clone(ErrorResult &aRv) const
{
  RefPtr<Label> label = new Label();

  if(!label) {
    aRv = NS_ERROR_OUT_OF_MEMORY;
    return nullptr;
  }

  DisjunctionSetArray *newRoles = label->GetDirectRoles();
  for (unsigned i = 0; i < mRoles.Length(); i++) {
    DisjunctionSet dset = DisjunctionSetUtils::CloneDset(mRoles[i]);
    // Role* role = mRoles[i]->Clone(aRv);
    newRoles->InsertElementAt(i, dset);
  }

  return label.forget();
}

already_AddRefed<Label>
Label::Upgrade(mozilla::dom::Label& privilegeLabel)
{
  ErrorResult aRv;
  // do an and and return...
  return this->And(privilegeLabel, aRv);
}

// TODO, maybe pass privilege instead
already_AddRefed<Label>
Label::Downgrade(mozilla::dom::Label& privilegeLabel)
{
  // Label& privLabel = const_cast<Label&>(privilegeLabel);
  // Label* privilegeLabel = priv.DirectLabel();
  RefPtr<Label> newLabel = new Label();

  ErrorResult aRv;
  int mRolesLength = mRoles.Length();
  for (unsigned i = 0; i < mRolesLength; i++) {
    RefPtr<Label> curr = new Label(mRoles[i], aRv);

    if (!privilegeLabel.Subsumes(*curr)) {
      // use the internal AND mRoles[i]
      newLabel->_And(mRoles[i], aRv);
    }

  }
  return newLabel.forget();
}

void
Label::Stringify(nsString& retval)
{
  int mRolesLength = mRoles.Length();

  // if empty label
  if (IsEmpty()) {
    retval = NS_LITERAL_STRING("'none'");
    return;
  }

  // TODO could probably be more succint with ternaries... For some reason it was not liked by NS_LITERAL_STRING however.
  if (mRolesLength > 1) retval = NS_LITERAL_STRING("(");
  else retval = NS_LITERAL_STRING("");

  for (unsigned i = 0; i < mRolesLength; i++) {
    nsAutoString role;
    DisjunctionSetUtils::Stringify(mRoles[i], role);
    retval.Append(role);
    if (i != (mRoles.Length() -1))
      retval.Append(NS_LITERAL_STRING(") AND ("));
  }

  if (mRolesLength > 1) retval.Append(NS_LITERAL_STRING(")"));
  else retval.Append(NS_LITERAL_STRING(""));
}

bool
Label::Subsumes(const DisjunctionSet &role,
                const mozilla::dom::Label& other)
{
  // Break out early if the other and this are the same.
  // Or the other is an empty label.
  if (&other == this || other.IsEmpty())
    return true;

  // if empty
  if (!role.Length())
    return Subsumes(other);

  ErrorResult aRv;
  RefPtr<Label> privs =
    new Label(const_cast<DisjunctionSet&>(role), aRv);

  if (aRv.Failed())
    return Subsumes(other);

  return Subsumes(*privs, other);
}

bool
Label::Subsumes(const mozilla::dom::Label &privs,
                const mozilla::dom::Label& other)
{
  // Break out early if the other and this are the same.
  // Or the other is an empty label.
  if (&other == this || other.IsEmpty())
    return true;

  ErrorResult aRv;
  RefPtr<Label> _this = Clone(aRv);
  if (aRv.Failed())
    return false;
  _this->_And(const_cast<mozilla::dom::Label&>(privs), aRv);
  if (aRv.Failed())
    return false;
  return _this->Subsumes(other);
}

bool
Label::ContainsSensitivePrincipal()
{
  for (unsigned i = 0; i < mRoles.Length(); ++i) {
    DisjunctionSet& role = mRoles.ElementAt(i);
    for (unsigned j = 0; j < role.Length(); ++j) {
      if (role[j].IsSensitivePrincipal()) return true;
    }

  }

  return false;
}

void
Label::_And(nsIPrincipal *p, ErrorResult& aRv)
{
  COWLPrincipal newPrincipal = COWLPrincipalUtils::ConstructPrincipal(p, aRv);
  DisjunctionSet newDSet = DisjunctionSetUtils::ConstructDset(newPrincipal);

  _And(newDSet, aRv);
}

void
Label::_And(DisjunctionSet& role, ErrorResult& aRv)
{
  InternalAnd(role, &aRv, true);
}

void
Label::_And(mozilla::dom::Label& label, ErrorResult& aRv)
{
  DisjunctionSetArray *otherRoles = label.GetDirectRoles();
  for (unsigned i = 0; i < otherRoles->Length(); ++i) {
    _And(otherRoles->ElementAt(i), aRv);
    if (aRv.Failed()) return;
  }
}

void
Label::_Or(DisjunctionSet& role, ErrorResult& aRv)
{
  /* Create a new label to which we'll add new roles containing the
   * above role. This eliminates the need to first do the disjunction
   * and then reduce the label to conjunctive normal form. */
  if (IsEmpty()) {
    DisjunctionSet emptyDset = DisjunctionSetUtils::ConstructDset();
    mRoles.AppendElement(emptyDset);
  }

  Label tmpLabel;

  for (unsigned i = 0; i < mRoles.Length(); ++i) {
    DisjunctionSet& nRole = mRoles.ElementAt(i);
    DisjunctionSetUtils::Or(nRole, role);

    tmpLabel.InternalAnd(nRole);
  }
  // copy assignment
  mRoles = *(tmpLabel.GetDirectRoles());
}

void
Label::_Or(mozilla::dom::Label& label, ErrorResult& aRv)
{
  DisjunctionSetArray *otherRoles = label.GetDirectRoles();
  for (unsigned i = 0; i < otherRoles->Length(); ++i) {
    _Or(otherRoles->ElementAt(i), aRv);
    if (aRv.Failed()) return;
  }
}

void
Label::Reduce(mozilla::dom::Label &label)
{
  if (label.IsEmpty()) return;

  DisjunctionSetArray *roles = const_cast<mozilla::dom::Label&>(label).GetDirectRoles();
  RoleSubsumeInvComparator cmp;
  for (unsigned i = 0; i < roles->Length(); ++i) {
    while (mRoles.RemoveElement(roles->ElementAt(i), cmp)) ;
  }
}

//
// Internals
//

bool
Label::Equals(mozilla::dom::Label& other)
{
  // Break out early if the other and this are the same.
  if (&other == this)
    return true;

  DisjunctionSetArray *otherRoles = other.GetDirectRoles();

  // The other label is of a different size.
  if (otherRoles->Length() != mRoles.Length())
    return false;

  RoleComparator cmp;
  for (unsigned i = 0; i<mRoles.Length(); ++i) {
    /* This label contains a role that the other label does not, hence
     * they cannot be equal. */
    if (!otherRoles->Contains(mRoles[i],cmp))
      return false;
  }

  return true;
}


void
Label::InternalAnd(DisjunctionSet& role, ErrorResult* aRv, bool clone)
{
  /* If there is no role in this label that subsumes |role|, append it
   * and remove any roles it subsumes.  An empty role is ignored.  */
  if (!mRoles.Contains(role, RoleSubsumeComparator())) {
    RoleSubsumeInvComparator cmp;

    // Remove any elements that this role subsumes
    while(mRoles.RemoveElement(role, cmp)) ;

    if (clone && aRv) {
      DisjunctionSet copyDset = DisjunctionSetUtils::CloneDset(role);

      mRoles.AppendElement(copyDset);
    } else {
      mRoles.AppendElement(role);
    }
  }
}

// HELPERS

COWLPrincipal
COWLPrincipalUtils::ConstructPrincipal(const nsAString& principal, ErrorResult& aRv)
{
  COWLPrincipalType principalState = COWLParser::validateFormat(principal);

  if (principalState == COWLPrincipalType::INVALID_PRINCIPAL) {
    aRv.ThrowTypeError<MSG_INVALID_PRINCIPAL>(principal);
  }

  if (principalState != COWLPrincipalType::ORIGIN_PRINCIPAL) {
    COWLPrincipal newPrincipal(principal, principalState);
    return newPrincipal;
  } else {
    COWLPrincipal newPrincipal = COWLPrincipalUtils::ConstructOriginPrincipal(principal);
    return newPrincipal;
  }

}

COWLPrincipal
COWLPrincipalUtils::ConstructPrincipal(nsIPrincipal *principal, ErrorResult& aRv)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = principal->GetURI(getter_AddRefs(uri));

  nsAutoCString origin1;
  uri->GetAsciiSpec(origin1);

  rv = principal->GetOrigin(origin1);

  // create a new Disjunction set...
  // COWLPrincipal newPrincipal(, COWLPrincipalType::ORIGIN_PRINCIPAL);

  return COWLPrincipalUtils::ConstructOriginPrincipal(NS_ConvertASCIItoUTF16(origin1));
}

COWLPrincipal
COWLPrincipalUtils::ConstructOriginPrincipal(const nsAString& principal)
{
  nsAutoString origin;
  origin.Assign(principal);
  // try to normalize origin, if that fails use passed in string.
  nsresult rv;
  // Create URI
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), principal);

  if (NS_SUCCEEDED(rv)) {
    nsAutoCString tmpOrigin;
    rv = uri->GetAsciiSpec(tmpOrigin);
    CopyASCIItoUTF16(tmpOrigin, origin);
  }

  COWLPrincipal newPrincipal(origin, COWLPrincipalType::ORIGIN_PRINCIPAL);
  return newPrincipal;
}

DisjunctionSet
DisjunctionSetUtils::ConstructDset()
{
  DisjunctionSet dset;
  return dset;
}

DisjunctionSet
DisjunctionSetUtils::ConstructDset(COWLPrincipal& principal)
{
  DisjunctionSet dset;
  // insert into dset...
  PrincipalComparator cmp;
  dset.InsertElementSorted(principal, cmp);
  return dset;
}

DisjunctionSet
DisjunctionSetUtils::CloneDset(const DisjunctionSet& dset)
{
  DisjunctionSet copyDset;
  for (unsigned i = 0; i < dset.Length(); ++i) {
    copyDset.InsertElementAt(i, dset[i]);
  }

  return copyDset;
}

void
DisjunctionSetUtils::Or(DisjunctionSet& dset1, DisjunctionSet& dset2)
{
  PrincipalComparator cmp;

  for (unsigned i = 0; i < dset2.Length(); i++) {
    COWLPrincipal& principal = dset2.ElementAt(i);
    if(!dset1.Contains(principal, cmp)) {
      dset1.InsertElementSorted(principal, cmp);
    }

  }
}

bool
DisjunctionSetUtils::ContainsOriginPrincipal(const DisjunctionSet& dset)
{
  // go through..
  bool foundOriginPrincipal = false;

  for (unsigned i = 0; i < dset.Length(); ++i) {
    if (dset[i].IsOriginPrincipal()) {
      foundOriginPrincipal = true;
      break;
    }
  }

  return foundOriginPrincipal;
}

bool
DisjunctionSetUtils::Equals(const DisjunctionSet& dset1, const DisjunctionSet& dset2)
{
  // Break out early if the other and this are the same.
  if (&dset1 == &dset2)
    return true;

  // The other role is of a different size, can't be equal.
  if (dset2.Length() != dset1.Length())
    return false;

  PrincipalComparator cmp;
  for (unsigned i=0; i< dset1.Length(); ++i) {
    /* This role contains a principal that the other role does not,
     * hence it cannot be equal to it. */
    if(!cmp.Equals(dset1[i], dset2[i]))
      return false;
  }

  return true;
}

bool
DisjunctionSetUtils::Subsumes(const DisjunctionSet& dset1, const DisjunctionSet& dset2)
{
  // Break out early if the other points to this
  if (&dset1 == &dset2)
    return true;

  // PrincipalArray *otherPrincipals = const_cast<mozilla::dom::Role&>(other).GetDirectPrincipals();

  // dset2 (Disjunction set) is smaller, dset1 cannot imply (subsume) it.
  if (dset2.Length() < dset1.Length())
    return false;

  PrincipalComparator cmp;
  for (unsigned i=0; i< dset1.Length(); ++i) {
    /* This role contains a principal that the other role does not,
     * hence it cannot imply (subsume) it. */
    if (!dset2.Contains(dset1[i],cmp))
      return false;
  }

  return true;
}

void
DisjunctionSetUtils::Stringify(const DisjunctionSet& dset, nsString& retval)
{
  retval = NS_LITERAL_STRING("");

  for (unsigned i=0; i < dset.Length(); ++i) {
    const COWLPrincipal& principal = dset[i];
    nsAutoString principalString;
    principal.Stringify(principalString);

    retval.Append(principalString);

    if (i != (dset.Length() - 1))
      retval.Append(NS_LITERAL_STRING(" OR "));
  }

  retval.Append(NS_LITERAL_STRING(""));
}


int
PrincipalComparator::Compare(const COWLPrincipal &p1,
                                const COWLPrincipal &p2) const
{
  nsAutoString p1String;
  p1.Stringify(p1String);
  nsAutoString p2String;
  p2.Stringify(p2String);

  bool res = strcmp(ToNewUTF8String(p1String), ToNewUTF8String(p2String));
  return res;
}


} // namespace dom
} // namespace mozilla
