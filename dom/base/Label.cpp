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
}

Label::Label(mozilla::dom::Role &role, ErrorResult &aRv)
{
  _And(role,aRv);
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
                   ErrorResult& aRv)
{
  Role* role = new Role(principal, aRv);
  if (aRv.Failed()) return nullptr;

  RefPtr<Label> label = new Label(*role, aRv);
  if (aRv.Failed())
    return nullptr;
  return label.forget();
}

bool
Label::Equals(mozilla::dom::Label& other)
{
  // Break out early if the other and this are the same.
  if (&other == this)
    return true;

  RoleArray *otherRoles = other.GetDirectRoles();

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

  RoleArray *otherRoles =
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
  Role* role = new Role(principal, aRv);
  if (aRv.Failed())
    return nullptr;

  RefPtr<Label> _this = Clone(aRv);
  if (aRv.Failed())
    return nullptr;

  _this->_And(*role, aRv);
  if (aRv.Failed())
    return nullptr;

  return _this.forget();
}

already_AddRefed<Label>
Label::And(mozilla::dom::Role& role, ErrorResult& aRv)
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

  Role* role = new Role(principal, aRv);
  if (aRv.Failed())
    return nullptr;

  _this->_Or(*role, aRv);
  if (aRv.Failed())
    return nullptr;

  return _this.forget();
}


already_AddRefed<Label>
Label::Or(mozilla::dom::Role& role, ErrorResult& aRv)
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

  RoleArray *newRoles = label->GetDirectRoles();
  for (unsigned i = 0; i < mRoles.Length(); i++) {
    Role* role = mRoles[i]->Clone(aRv);
    if (aRv.Failed())
      return nullptr;
    newRoles->InsertElementAt(i, role);
  }

  return label.forget();
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
    RefPtr<Label> curr = new Label(*mRoles[i], aRv);

    if (!privilegeLabel.Subsumes(*curr)) {
      // use the internal AND mRoles[i]
      newLabel->_And(*mRoles[i], aRv);
    }

  }
  return newLabel.forget();
}

void
Label::Stringify(nsString& retval)
{
  int mRolesLength = mRoles.Length();

  // if empty label
  if (mRolesLength == 0) {
    retval = NS_LITERAL_STRING("'none'");
    return;
  }

  // TODO could probably be more succint with ternaries... For some reason it was not liked by NS_LITERAL_STRING however.
  if (mRolesLength > 1) retval = NS_LITERAL_STRING("(");
  else retval = NS_LITERAL_STRING("");

  for (unsigned i = 0; i < mRolesLength; i++) {
    nsAutoString role;
    mRoles[i]->Stringify(role);
    retval.Append(role);
    if (i != (mRoles.Length() -1))
      retval.Append(NS_LITERAL_STRING(") AND ("));
  }

  if (mRolesLength > 1) retval.Append(NS_LITERAL_STRING(")"));
  else retval.Append(NS_LITERAL_STRING(""));
}

// bool
// Label::Subsumes(nsIPrincipal* priv, const mozilla::dom::Label& other)
// {
//   if (!priv)
//     return Subsumes(other);

//   RefPtr<Role> privRole = new Role(priv);
//   return Subsumes(*privRole,other);
// }

bool
Label::Subsumes(const mozilla::dom::Role &role,
                const mozilla::dom::Label& other)
{
  // Break out early if the other and this are the same.
  // Or the other is an empty label.
  if (&other == this || other.IsEmpty())
    return true;

  if (role.IsEmpty())
    return Subsumes(other);

  ErrorResult aRv;
  RefPtr<Label> privs =
    new Label(const_cast<mozilla::dom::Role &>(role), aRv);

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

void
Label::_And(nsIPrincipal *p, ErrorResult& aRv)
{
  Role* role = new Role(p);
  _And(*role, aRv);
}

void
Label::_And(mozilla::dom::Role& role, ErrorResult& aRv)
{
  InternalAnd(role, &aRv, true);
}

void
Label::_And(mozilla::dom::Label& label, ErrorResult& aRv)
{
  RoleArray *otherRoles = label.GetDirectRoles();
  for (unsigned i = 0; i < otherRoles->Length(); ++i) {
    _And(*(otherRoles->ElementAt(i)), aRv);
    if (aRv.Failed()) return;
  }
}

void
Label::_Or(mozilla::dom::Role& role, ErrorResult& aRv)
{

  // This label is empty, disjunction should not change it.
  if (IsEmpty())
    return;

  /* Create a new label to which we'll add new roles containing the
   * above role. This eliminates the need to first do the disjunction
   * and then reduce the label to conjunctive normal form. */

  Label tmpLabel;

  for (unsigned i = 0; i < mRoles.Length(); ++i) {
    Role* nRole = mRoles.ElementAt(i);
    nRole->_Or(role);

    tmpLabel.InternalAnd(*nRole);
  }
  // copy assignment
  mRoles = *(tmpLabel.GetDirectRoles());
}

void
Label::_Or(mozilla::dom::Label& label, ErrorResult& aRv)
{
  RoleArray *otherRoles = label.GetDirectRoles();
  for (unsigned i = 0; i < otherRoles->Length(); ++i) {
    _Or(*(otherRoles->ElementAt(i)), aRv);
    if (aRv.Failed()) return;
  }
}

void
Label::Reduce(mozilla::dom::Label &label)
{
  if (label.IsEmpty()) return;

  RoleArray *roles = const_cast<mozilla::dom::Label&>(label).GetDirectRoles();
  RoleSubsumeInvComparator cmp;
  for (unsigned i = 0; i < roles->Length(); ++i) {
    while (mRoles.RemoveElement(roles->ElementAt(i), cmp)) ;
  }
}

// already_AddRefed<nsIPrincipal>
// Label::GetPrincipalIfSingleton() const
// {
//   PrincipalsArray* ps = GetPrincipalsIfSingleton();

//   if (!ps || ps->Length() != 1)
//     return nullptr;

//   nsCOMPtr<nsIPrincipal> p = ps->ElementAt(0);
//   return p.forget();
// }

PrincipalArray*
Label::GetPrincipalsIfSingleton() const
{
  if (mRoles.Length() != 1)
    return nullptr;
  return mRoles.ElementAt(0)->GetDirectPrincipals();
}

//
// Internals
//

void
Label::InternalAnd(mozilla::dom::Role& role, ErrorResult* aRv, bool clone)
{
  /* If there is no role in this label that subsumes |role|, append it
   * and remove any roles it subsumes.  An empty role is ignored.  */
  if (!mRoles.Contains(&role, RoleSubsumeComparator())) {
    RoleSubsumeInvComparator cmp;

    // Remove any elements that this role subsumes
    while(mRoles.RemoveElement(&role, cmp)) ;

    if (clone && aRv) {
      mozilla::dom::Role* roleCopy = role.Clone(*aRv);
      if(!roleCopy)
        return;

      mRoles.AppendElement(roleCopy);
    } else {
      mRoles.AppendElement(&role);
    }
  }
}


} // namespace dom
} // namespace mozilla
