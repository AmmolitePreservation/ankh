#include "accountView.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

AccountView::AccountView(const QString& serviceAddress, QWidget* parent)
  : QWidget(parent),
    states(new QStackedWidget(this)),
    note(new QLabel(this)),
    signedInName(new QLabel(this)),
    signedInRole(new QLabel(this))
{
  auto* heading = new QLabel(QStringLiteral("Ammolite account"), this);
  heading->setProperty("heading", true);
  auto* address = new QLabel(QStringLiteral("Service: %1").arg(serviceAddress), this);
  address->setProperty("secondary", true);
  address->setTextInteractionFlags(Qt::TextSelectableByMouse);

  note->setProperty("bodySecondary", true);
  note->setWordWrap(true);
  auto* signInButton = new QPushButton(QStringLiteral("Sign in"), this);
  signInButton->setObjectName(QStringLiteral("primary"));
  auto* createButton = new QPushButton(QStringLiteral("Create an account"), this);

  auto* signedOut = new QWidget(this);
  auto* signedOutLayout = new QVBoxLayout(signedOut);
  signedOutLayout->setContentsMargins(0, 0, 0, 0);
  signedOutLayout->setSpacing(16);
  signedOutLayout->addWidget(note);
  auto* actions = new QHBoxLayout;
  actions->setSpacing(12);
  actions->addWidget(signInButton);
  actions->addWidget(createButton);
  actions->addStretch(1);
  signedOutLayout->addLayout(actions);
  signedOutLayout->addStretch(1);

  auto* signedIn = new QWidget(this);
  auto* signedInLayout = new QVBoxLayout(signedIn);
  signedInLayout->setContentsMargins(0, 0, 0, 0);
  signedInLayout->setSpacing(16);
  auto* details = new QFormLayout;
  details->setHorizontalSpacing(24);
  details->setVerticalSpacing(10);
  details->addRow(QStringLiteral("Signed in as"), signedInName);
  details->addRow(QStringLiteral("Role"), signedInRole);
  signedInLayout->addLayout(details);
  auto* signOutButton = new QPushButton(QStringLiteral("Sign out"), signedIn);
  signedInLayout->addWidget(signOutButton, 0, Qt::AlignLeft);
  signedInLayout->addStretch(1);

  states->addWidget(signedOut);
  states->addWidget(signedIn);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(32, 28, 32, 0);
  layout->setSpacing(16);
  layout->addWidget(heading);
  layout->addWidget(address);
  layout->addWidget(states, 1);

  connect(signInButton, &QPushButton::clicked, this, &AccountView::signInRequested);
  connect(createButton, &QPushButton::clicked, this, &AccountView::createAccountRequested);
  connect(signOutButton, &QPushButton::clicked, this, &AccountView::signOutRequested);
}

void AccountView::showSignedOut(const QString& message)
{
  note->setText(message.isEmpty() ? QStringLiteral("You are not signed in.") : message);
  states->setCurrentIndex(0);
}

void AccountView::showSignedIn(const SignedInUser& user)
{
  signedInName->setText(user.username);
  signedInRole->setText(user.role);
  states->setCurrentIndex(1);
}
