///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <gui/GUI.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/scene/ObjectNode.h>
#include <core/dataset/scene/SelectionSet.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/dataset/UndoStack.h>
#include <core/dataset/DataSetContainer.h>
#include <core/app/Application.h>
#include <gui/actions/ActionManager.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/widgets/selection/SceneNodeSelectionBox.h>
#include "ModifyCommandPage.h"
#include "PipelineListModel.h"
#include "ModifierListBox.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Initializes the modify page.
******************************************************************************/
ModifyCommandPage::ModifyCommandPage(MainWindow* mainWindow, QWidget* parent) : QWidget(parent),
		_datasetContainer(mainWindow->datasetContainer()), _actionManager(mainWindow->actionManager())
{
	QGridLayout* layout = new QGridLayout(this);
	layout->setContentsMargins(2,2,2,2);
	layout->setSpacing(4);
	layout->setColumnStretch(1,1);

	SceneNodeSelectionBox* nodeSelBox = new SceneNodeSelectionBox(_datasetContainer, this);
	layout->addWidget(nodeSelBox, 0, 0, 1, 2);

	_pipelineListModel = new PipelineListModel(_datasetContainer, this);
	_modifierSelector = new ModifierListBox(this, _pipelineListModel);
    layout->addWidget(_modifierSelector, 1, 0, 1, 2);
    connect(_modifierSelector, (void (QComboBox::*)(int))&QComboBox::activated, this, &ModifyCommandPage::onModifierAdd);

	class PipelineListView : public QListView {
	public:
		PipelineListView(QWidget* parent) : QListView(parent) {}
		virtual QSize sizeHint() const { return QSize(256, 260); }
	};

	QSplitter* splitter = new QSplitter(Qt::Vertical);
	splitter->setChildrenCollapsible(false);

	QWidget* upperContainer = new QWidget();
	splitter->addWidget(upperContainer);
	QHBoxLayout* subLayout = new QHBoxLayout(upperContainer);
	subLayout->setContentsMargins(0,0,0,0);
	subLayout->setSpacing(2);

	_pipelineWidget = new PipelineListView(upperContainer);
	_pipelineWidget->setDragDropMode(QAbstractItemView::InternalMove);
	_pipelineWidget->setDragEnabled(true);
	_pipelineWidget->setAcceptDrops(true);
	_pipelineWidget->setDragDropOverwriteMode(false);
	_pipelineWidget->setDropIndicatorShown(true);
	_pipelineWidget->setModel(_pipelineListModel);
	_pipelineWidget->setSelectionModel(_pipelineListModel->selectionModel());
	connect(_pipelineListModel, &PipelineListModel::selectedItemChanged, this, &ModifyCommandPage::onSelectedItemChanged);
	connect(_pipelineWidget, &PipelineListView::doubleClicked, this, &ModifyCommandPage::onModifierStackDoubleClicked);
	subLayout->addWidget(_pipelineWidget);

	QToolBar* editToolbar = new QToolBar(this);
	editToolbar->setOrientation(Qt::Vertical);
#ifndef Q_OS_MACX
	editToolbar->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; }");
#endif
	subLayout->addWidget(editToolbar);

	QAction* deleteModifierAction = _actionManager->createCommandAction(ACTION_MODIFIER_DELETE, tr("Delete Modifier"), ":/gui/actions/modify/delete_modifier.png");
	connect(deleteModifierAction, &QAction::triggered, this, &ModifyCommandPage::onDeleteModifier);
	editToolbar->addAction(deleteModifierAction);

	editToolbar->addSeparator();

	QAction* moveModifierUpAction = _actionManager->createCommandAction(ACTION_MODIFIER_MOVE_UP, tr("Move Modifier Up"), ":/gui/actions/modify/modifier_move_up.png");
	connect(moveModifierUpAction, &QAction::triggered, this, &ModifyCommandPage::onModifierMoveUp);
	editToolbar->addAction(moveModifierUpAction);
	QAction* moveModifierDownAction = mainWindow->actionManager()->createCommandAction(ACTION_MODIFIER_MOVE_DOWN, tr("Move Modifier Down"), ":/gui/actions/modify/modifier_move_down.png");
	connect(moveModifierDownAction, &QAction::triggered, this, &ModifyCommandPage::onModifierMoveDown);
	editToolbar->addAction(moveModifierDownAction);

	QAction* toggleModifierStateAction = _actionManager->createCommandAction(ACTION_MODIFIER_TOGGLE_STATE, tr("Enable/Disable Modifier"));
	toggleModifierStateAction->setCheckable(true);
	QIcon toggleStateActionIcon(QString(":/gui/actions/modify/modifier_enabled_large.png"));
	toggleStateActionIcon.addFile(QString(":/gui/actions/modify/modifier_disabled_large.png"), QSize(), QIcon::Normal, QIcon::On);
	toggleModifierStateAction->setIcon(toggleStateActionIcon);
	connect(toggleModifierStateAction, &QAction::triggered, this, &ModifyCommandPage::onModifierToggleState);

	editToolbar->addSeparator();
	QAction* createCustomModifierAction = _actionManager->createCommandAction(ACTION_MODIFIER_CREATE_PRESET, tr("Save Modifier Preset"), ":/gui/actions/modify/modifier_save_preset.png");
	connect(createCustomModifierAction, &QAction::triggered, this, &ModifyCommandPage::onCreateCustomModifier);
	editToolbar->addAction(createCustomModifierAction);

	editToolbar->addSeparator();
	QAction* helpAction = new QAction(QIcon(":/gui/mainwin/command_panel/help.png"), tr("Open Online Help"), this);
	connect(helpAction, &QAction::triggered, [mainWindow] {
		mainWindow->openHelpTopic("usage.modification_pipeline.html");
	});
	editToolbar->addAction(helpAction);

	layout->addWidget(splitter, 2, 0, 1, 2);
	layout->setRowStretch(2, 1);

	// Create the properties panel.
	_propertiesPanel = new PropertiesPanel(nullptr, mainWindow);
	_propertiesPanel->setFrameStyle(QFrame::NoFrame | QFrame::Plain);
	splitter->addWidget(_propertiesPanel);
	splitter->setStretchFactor(1,1);

	connect(&_datasetContainer, &DataSetContainer::selectionChangeComplete, this, &ModifyCommandPage::onSelectionChangeComplete);
	updateActions(nullptr);

	// Create About panel.
	createAboutPanel();
}

/******************************************************************************
* This is called after all changes to the selection set have been completed.
******************************************************************************/
void ModifyCommandPage::onSelectionChangeComplete(SelectionSet* newSelection)
{
	// Make sure we get informed about any future changes of the selection set.
	_pipelineListModel->refreshList();
}

/******************************************************************************
* Is called when a new modification list item has been selected, or if the currently
* selected item has changed.
******************************************************************************/
void ModifyCommandPage::onSelectedItemChanged()
{
	PipelineListItem* currentItem = _pipelineListModel->selectedItem();
	RefTarget* object = currentItem ? currentItem->object() : nullptr;

	if(currentItem != nullptr)
		_aboutRollout->hide();

	if(object != _propertiesPanel->editObject()) {
		_propertiesPanel->setEditObject(object);
		if(_datasetContainer.currentSet())
			_datasetContainer.currentSet()->viewportConfig()->updateViewports();
	}
	updateActions(currentItem);

	// Whenever no object is selected, show the About Panel containing information about the program.
	if(currentItem == nullptr)
		_aboutRollout->show();
}

/******************************************************************************
* Updates the state of the actions that can be invoked on the currently selected item.
******************************************************************************/
void ModifyCommandPage::updateActions(PipelineListItem* currentItem)
{
	QAction* deleteModifierAction = _actionManager->getAction(ACTION_MODIFIER_DELETE);
	QAction* moveModifierUpAction = _actionManager->getAction(ACTION_MODIFIER_MOVE_UP);
	QAction* moveModifierDownAction = _actionManager->getAction(ACTION_MODIFIER_MOVE_DOWN);
	QAction* toggleModifierStateAction = _actionManager->getAction(ACTION_MODIFIER_TOGGLE_STATE);
	QAction* createCustomModifierAction = _actionManager->getAction(ACTION_MODIFIER_CREATE_PRESET);

	_modifierSelector->setEnabled(currentItem != nullptr);

	Modifier* modifier = currentItem ? dynamic_object_cast<Modifier>(currentItem->object()) : nullptr;
	if(modifier) {
		deleteModifierAction->setEnabled(true);
		createCustomModifierAction->setEnabled(true);
		if(currentItem->modifierApplications().size() == 1) {
			ModifierApplication* modApp = currentItem->modifierApplications()[0];
			moveModifierDownAction->setEnabled(dynamic_object_cast<ModifierApplication>(modApp->input()) != nullptr);
			int index = _pipelineListModel->items().indexOf(currentItem);
			PipelineListItem* predecessor = (index > 0) ? _pipelineListModel->item(index - 1) : nullptr;
			moveModifierUpAction->setEnabled(dynamic_object_cast<Modifier>(predecessor ? predecessor->object() : nullptr) != nullptr);
		}
		else {
			moveModifierUpAction->setEnabled(false);
			moveModifierDownAction->setEnabled(false);
		}
		if(modifier) {
			toggleModifierStateAction->setEnabled(true);
			toggleModifierStateAction->setChecked(modifier->isEnabled() == false);
		}
		else {
			toggleModifierStateAction->setChecked(false);
			toggleModifierStateAction->setEnabled(false);
		}
	}
	else {
		deleteModifierAction->setEnabled(false);
		moveModifierUpAction->setEnabled(false);
		moveModifierDownAction->setEnabled(false);
		toggleModifierStateAction->setChecked(false);
		toggleModifierStateAction->setEnabled(false);
		createCustomModifierAction->setEnabled(false);
	}
}

/******************************************************************************
* Is called when the user has selected an item in the modifier class list.
******************************************************************************/
void ModifyCommandPage::onModifierAdd(int index)
{
	if(index >= 0 && _pipelineListModel->isUpToDate()) {
		ModifierClassPtr modifierClass = _modifierSelector->itemData(index).value<ModifierClassPtr>();
		if(modifierClass) {
			UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Apply modifier"), [modifierClass, this]() {
				// Create an instance of the modifier.
				OORef<Modifier> modifier = static_object_cast<Modifier>(modifierClass->createInstance(_datasetContainer.currentSet()));
				OVITO_CHECK_OBJECT_POINTER(modifier);
				// Load user-defined default parameters.
				modifier->loadUserDefaults();
				// Apply it.
				_pipelineListModel->applyModifiers({modifier});
			});
			_pipelineListModel->requestUpdate();
		}
		else {
			QString presetName = _modifierSelector->itemData(index).toString();
			if(!presetName.isEmpty()) {
				UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Apply modifier set"), [presetName, this]() {
					// Deserialize modifier set from settings store.
					QVector<OORef<Modifier>> modifierSet;
					try {
						UndoSuspender noUndo(_datasetContainer.currentSet()->undoStack());
						QSettings settings;
						settings.beginGroup("core/modifier/presets/");
						QByteArray buffer = settings.value(presetName).toByteArray();
						QDataStream dstream(buffer);
						ObjectLoadStream stream(dstream);
						stream.setDataset(_datasetContainer.currentSet());
						for(int chunkId = stream.expectChunkRange(0,1); chunkId == 1; chunkId = stream.expectChunkRange(0,1)) {
							modifierSet.push_back(stream.loadObject<Modifier>());
							stream.closeChunk();
						}
						stream.closeChunk();
						stream.close();
					}
					catch(Exception& ex) {
						if(ex.context() == nullptr) ex.setContext(_datasetContainer.currentSet());
						ex.prependGeneralMessage(tr("Failed to load stored modifier set."));
						throw;
					}
					_pipelineListModel->applyModifiers(modifierSet);
				});
				_pipelineListModel->requestUpdate();
			}
		}

		_modifierSelector->setCurrentIndex(0);
	}
}

/******************************************************************************
* Handles the ACTION_MODIFIER_DELETE command.
******************************************************************************/
void ModifyCommandPage::onDeleteModifier()
{
	// Get the currently selected modifier.
	PipelineListItem* selectedItem = _pipelineListModel->selectedItem();
	if(!selectedItem) return;

	Modifier* modifier = dynamic_object_cast<Modifier>(selectedItem->object());
	if(!modifier) return;

	UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Delete modifier"), [selectedItem, modifier, this]() {

		// Determine the modifier item preceding the item to be deleted.
		int index = _pipelineListModel->items().indexOf(selectedItem);
		while(index > 0) {
			PipelineListItem* predecessor = _pipelineListModel->items()[index-1];
			if(dynamic_object_cast<Modifier>(predecessor->object())) {
				for(ModifierApplication* modApp : predecessor->modifierApplications()) {
					ModifierApplication* toBeDeleted = dynamic_object_cast<ModifierApplication>(modApp->input());
					OVITO_ASSERT(selectedItem->modifierApplications().contains(toBeDeleted));
					if(toBeDeleted) {
						modApp->setInput(toBeDeleted->input());
						_pipelineListModel->setNextToSelectObject(modApp->input());
					}
				}
				break;
			}
			index--;
		}

		if(index <= 0) {
			for(ObjectNode* objNode : _pipelineListModel->selectedNodes()) {
				ModifierApplication* toBeDeleted = dynamic_object_cast<ModifierApplication>(objNode->dataProvider());
				if(toBeDeleted) {
					objNode->setDataProvider(toBeDeleted->input());
					_pipelineListModel->setNextToSelectObject(objNode->dataProvider());
				}
			}
		}

		// Delete modifier if there are no more applications left.
		if(modifier->modifierApplications().empty())
			modifier->deleteReferenceObject();
	});
}

/******************************************************************************
* This called when the user double clicks on an item in the modifier stack.
******************************************************************************/
void ModifyCommandPage::onModifierStackDoubleClicked(const QModelIndex& index)
{
	PipelineListItem* item = _pipelineListModel->item(index.row());
	OVITO_CHECK_OBJECT_POINTER(item);

	Modifier* modifier = dynamic_object_cast<Modifier>(item->object());
	if(modifier) {
		// Toggle enabled state of modifier.
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Toggle modifier state"), [modifier]() {
			modifier->setEnabled(!modifier->isEnabled());
		});
	}

	DisplayObject* displayObj = dynamic_object_cast<DisplayObject>(item->object());
	if(displayObj) {
		// Toggle enabled state of display object.
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Toggle display state"), [displayObj]() {
			displayObj->setEnabled(!displayObj->isEnabled());
		});
	}
}

/******************************************************************************
* Handles the ACTION_MODIFIER_MOVE_UP command, which moves the selected
* modifier up one position in the stack.
******************************************************************************/
void ModifyCommandPage::onModifierMoveUp()
{
	// Get the currently selected modifier.
	PipelineListItem* selectedItem = _pipelineListModel->selectedItem();
	if(!selectedItem) return;

	if(selectedItem->modifierApplications().size() != 1)
		return;

	UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Move modifier up"), [selectedItem]() {
		QVector<ModifierApplication*> modApps = selectedItem->modifierApplications();
		for(OORef<ModifierApplication> modApp : modApps) {
			for(RefMaker* dependent : modApp->dependents()) {
				if(OORef<ModifierApplication> predecessor = dynamic_object_cast<ModifierApplication>(dependent)) {
					for(RefMaker* dependent2 : predecessor->dependents()) {
						if(ModifierApplication* predecessor2 = dynamic_object_cast<ModifierApplication>(dependent2)) {
							predecessor->setInput(modApp->input());
							predecessor2->setInput(modApp);
							modApp->setInput(predecessor);
							break;
						}
						else if(ObjectNode* predecessor2 = dynamic_object_cast<ObjectNode>(dependent2)) {
							predecessor->setInput(modApp->input());
							predecessor2->setDataProvider(modApp);
							modApp->setInput(predecessor);
							break;
						}
					}
					break;
				}				
			}
		}
	});
}

/******************************************************************************
* Handles the ACTION_MODIFIER_MOVE_DOWN command, which moves the selected
* modifier down one position in the stack.
******************************************************************************/
void ModifyCommandPage::onModifierMoveDown()
{
	// Get the currently selected modifier.
	PipelineListItem* selectedItem = _pipelineListModel->selectedItem();
	if(!selectedItem) return;

	if(selectedItem->modifierApplications().size() != 1)
		return;

	UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Move modifier down"), [selectedItem]() {
		QVector<ModifierApplication*> modApps = selectedItem->modifierApplications();
		for(OORef<ModifierApplication> modApp : modApps) {
			if(OORef<ModifierApplication> successor = dynamic_object_cast<ModifierApplication>(modApp->input())) {
				for(RefMaker* dependent : modApp->dependents()) {
					if(ModifierApplication* predecessor = dynamic_object_cast<ModifierApplication>(dependent)) {
						modApp->setInput(successor->input());
						successor->setInput(modApp);
						predecessor->setInput(successor);
						break;
					}
					else if(ObjectNode* predecessor = dynamic_object_cast<ObjectNode>(dependent)) {
						modApp->setInput(successor->input());
						successor->setInput(modApp);
						predecessor->setDataProvider(successor);
						break;
					}
				}
			}
		}
	});
}

/******************************************************************************
* Handles the ACTION_MODIFIER_TOGGLE_STATE command, which toggles the
* enabled/disable state of the selected modifier.
******************************************************************************/
void ModifyCommandPage::onModifierToggleState(bool newState)
{
	// Get the selected modifier from the modifier stack box.
	QModelIndexList selection = _pipelineWidget->selectionModel()->selectedRows();
	if(selection.empty())
		return;

	onModifierStackDoubleClicked(selection.front());
}

/******************************************************************************
* Creates the rollout panel that shows information about the application
* whenever no object is selected.
******************************************************************************/
void ModifyCommandPage::createAboutPanel()
{
	QWidget* rollout = new QWidget();
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(8,8,8,8);

	QTextBrowser* aboutLabel = new QTextBrowser(rollout);
	aboutLabel->setObjectName("AboutLabel");
	aboutLabel->setOpenExternalLinks(true);
	aboutLabel->setMinimumHeight(600);
	aboutLabel->setFrameStyle(QFrame::NoFrame | QFrame::Plain);
	aboutLabel->viewport()->setAutoFillBackground(false);
	layout->addWidget(aboutLabel);

	QByteArray newsPage;
#if !defined(OVITO_BUILD_APPSTORE_VERSION)
	QSettings settings;
	if(settings.value("updates/check_for_updates", true).toBool()) {
		// Retrieve cached news page from settings store.
		newsPage = settings.value("news/cached_webpage").toByteArray();
	}
	if(newsPage.isEmpty()) {
		QResource res("/gui/mainwin/command_panel/about_panel.html");
		newsPage = QByteArray((const char *)res.data(), (int)res.size());
	}
#else
	QResource res("/gui/mainwin/command_panel/about_panel_no_updates.html");
	newsPage = QByteArray((const char *)res.data(), (int)res.size());
#endif
	aboutLabel->setHtml(QString::fromUtf8(newsPage.constData()));

	_aboutRollout = _propertiesPanel->addRollout(rollout, QCoreApplication::applicationName());

#if !defined(OVITO_BUILD_APPSTORE_VERSION)
	if(settings.value("updates/check_for_updates", true).toBool()) {

		// Retrieve/generate unique installation id.
		QByteArray id;
		if(settings.value("updates/transmit_id", true).toBool()) {
			if(settings.contains("installation/id")) {
				id = settings.value("id").toByteArray();
				if(id == QByteArray(16, '\0') || id.size() != 16)
					id.clear();
			}
			if(id.isEmpty()) {
				// Generate a new unique ID.
				id.fill('0', 16);
				std::random_device rdev;
				std::uniform_int_distribution<> rdist(0, 0xFF);
				for(auto& c : id)
					c = (char)rdist(rdev);
				settings.setValue("installation/id", id);
			}
		}
		else {
			id.fill(0, 16);
		}

		QString operatingSystemString;
#if defined(Q_OS_MAC)
		operatingSystemString = QStringLiteral("macosx");
#elif defined(Q_OS_WIN)
		operatingSystemString = QStringLiteral("win");
#elif defined(Q_OS_LINUX)
		operatingSystemString = QStringLiteral("linux");
#endif

		// Fetch newest web page from web server.
		QNetworkAccessManager* networkAccessManager = new QNetworkAccessManager(_aboutRollout);
		QString urlString = QString("http://www.ovito.org/appnews/v%1.%2.%3/?ovito=%4&OS=%5%6")
				.arg(Application::applicationVersionMajor())
				.arg(Application::applicationVersionMinor())
				.arg(Application::applicationVersionRevision())
				.arg(QString(id.toHex()))
				.arg(operatingSystemString)
				.arg(QT_POINTER_SIZE*8);
		QNetworkReply* networkReply = networkAccessManager->get(QNetworkRequest(QUrl(urlString)));
		connect(networkAccessManager, &QNetworkAccessManager::finished, this, &ModifyCommandPage::onWebRequestFinished);
	}
#endif
}

/******************************************************************************
* Is called by the system when fetching the news web page from the server is
* completed.
******************************************************************************/
void ModifyCommandPage::onWebRequestFinished(QNetworkReply* reply)
{
	if(reply->error() == QNetworkReply::NoError) {
		QByteArray page = reply->readAll();
		reply->close();
		if(page.startsWith("<html><!--OVITO-->")) {

			QTextBrowser* aboutLabel = _aboutRollout->findChild<QTextBrowser*>("AboutLabel");
			OVITO_CHECK_POINTER(aboutLabel);
			aboutLabel->setHtml(QString::fromUtf8(page.constData()));

			QSettings settings;
			settings.setValue("news/cached_webpage", page);
		}
	}
	reply->deleteLater();
}

/******************************************************************************
* Handles the ACTION_MODIFIER_CREATE_PRESET command.
******************************************************************************/
void ModifyCommandPage::onCreateCustomModifier()
{
	QDialog dlg(window());
	dlg.setWindowTitle(tr("Save Modifier Preset"));
	QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(12);

	QLabel* label = new QLabel(tr(
			"This dialog window allows you to save the parameters of one or more modifiers from the current modification pipeline as a preset for future use. "
			"Tick those modifiers in the list below that you want to be included in the saved modifier set. "
			"Enter a name for the new preset, then click 'Save'. "
			"The new customized modifier will then appear in the list of available modifiers, following OVITO's standard modifiers."));
	label->setWordWrap(true);
	layout->addWidget(label);

	QListWidget* modifierListWidget = new QListWidget(&dlg);
	modifierListWidget->setUniformItemSizes(true);
	Modifier* selectedModifier = nullptr;
	QVector<Modifier*> modifierList;
	for(int index = 0; index < _pipelineListModel->rowCount(); index++) {
		PipelineListItem* item = _pipelineListModel->item(index);
		Modifier* modifier = dynamic_object_cast<Modifier>(item->object());
		if(modifier) {
			QListWidgetItem* listItem = new QListWidgetItem(modifier->objectTitle(), modifierListWidget);
			listItem->setFlags(Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren));
			if(_pipelineListModel->selectedItem() == item) {
				selectedModifier = modifier;
				listItem->setCheckState(Qt::Checked);
			}
			else listItem->setCheckState(Qt::Unchecked);
			modifierList.push_back(modifier);
		}
	}
	modifierListWidget->setMaximumHeight(modifierListWidget->sizeHintForRow(0) * qBound(3, modifierListWidget->count(), 10) + 2 * modifierListWidget->frameWidth());
	layout->addWidget(modifierListWidget);

	QGridLayout* sublayout = new QGridLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->setVerticalSpacing(2);
	sublayout->setHorizontalSpacing(10);
	QComboBox* nameBox = new QComboBox(&dlg);
	nameBox->setEditable(true);
	nameBox->setMinimumWidth(180);
	sublayout->addWidget(new QLabel(tr("Preset name:")), 0, 0);
	sublayout->addWidget(nameBox, 1, 0);
	sublayout->setColumnStretch(0, 1);
	QToolButton* deletePresetButton = new QToolButton();
	sublayout->addWidget(deletePresetButton, 1, 1);
	QAction* deletePresetAction = new QAction(QIcon(":/gui/actions/edit/edit_delete.png"), tr("Delete selected preset"), &dlg);
	deletePresetAction->setEnabled(false);
	deletePresetButton->setDefaultAction(deletePresetAction);
	sublayout->addWidget(new QLabel(tr("<p style=\"color: #686868; margin-right: 8;\"><span style=\"font-size: small;\">Use this button to delete an existing preset that you no longer need.</span> ") + QChar(0x2191) + QStringLiteral("</p>")), 2, 0, 1, 2, Qt::AlignRight);
	layout->addLayout(sublayout);

	connect(nameBox, &QComboBox::currentTextChanged, [nameBox, deletePresetAction](const QString& text) {
		deletePresetAction->setEnabled(nameBox->findText(text.trimmed()) != -1);
	});

	QSettings settings;
	settings.beginGroup("core/modifier/presets/");
	nameBox->addItems(settings.childKeys());

	connect(deletePresetAction, &QAction::triggered, [nameBox]() {
		QString presetName = nameBox->currentText().trimmed();
		int index = nameBox->findText(presetName);
		if(index != -1) {
			if(QMessageBox::question(nameBox, tr("Delete modifier preset"), tr("Do you really want to delete the existing modifier preset with the name '%1'?. This operation cannot be be undone.").arg(presetName), QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes)
				return;
			QSettings settings;
			settings.beginGroup("core/modifier/presets/");
			settings.remove(presetName);
			nameBox->removeItem(index);
			nameBox->clearEditText();
		}
	});

	if(selectedModifier)
		nameBox->setCurrentText(tr("Custom %1").arg(selectedModifier->objectTitle()));
	else
		nameBox->setCurrentText(tr("Custom modifier 1"));

	layout->setStretch(1, 1);
	mainLayout->addLayout(layout);
	mainLayout->setStretch(0, 1);

	mainLayout->addSpacing(12);
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
	connect(buttonBox, &QDialogButtonBox::accepted, [&dlg, nameBox, modifierListWidget]() {
		QString name = nameBox->currentText().trimmed();
		if(name.isEmpty()) {
			QMessageBox::critical(&dlg, tr("Save modifier preset"), tr("Please enter a name for the modifier set to be saved."));
			return;
		}
		if(nameBox->findText(name) != -1) {
			if(QMessageBox::question(&dlg, tr("Save modifier preset"), tr("A modifier preset with the name '%1' already exists. Do you want to replace it?").arg(name), QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes)
				return;
		}
		int selCount = 0;
		for(int i = 0; i < modifierListWidget->count(); i++)
			if(modifierListWidget->item(i)->checkState() == Qt::Checked)
				selCount++;
		if(!selCount) {
			QMessageBox::critical(&dlg, tr("Save modifier preset"), tr("Please check at least one modifier to be included in the saved set."));
			return;
		}
		dlg.accept();
	});
	connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

	// Implement Help button.
	MainWindow* mainWindow = _actionManager->mainWindow();
	connect(buttonBox, &QDialogButtonBox::helpRequested, [mainWindow]() {
		mainWindow->openHelpTopic(QStringLiteral("modifier_presets.html"));
	});

	mainLayout->addWidget(buttonBox);
	if(dlg.exec() == QDialog::Accepted) {
		try {
			// Serialize the selected modifiers to a byte array.
			QByteArray buffer;
			QDataStream dstream(&buffer, QIODevice::WriteOnly);
			ObjectSaveStream stream(dstream);
			for(int i = 0; i < modifierListWidget->count(); i++) {
				if(modifierListWidget->item(i)->checkState() == Qt::Checked) {
					stream.beginChunk(0x01);
					stream.saveObject(modifierList[i]);
					stream.endChunk();
				}
			}
			// Append EOF marker:
			stream.beginChunk(0x00);
			stream.endChunk();
			stream.close();

			// Save serialized modifier set in settings store.
			QSettings settings;
			settings.beginGroup("core/modifier/presets/");
			settings.setValue(nameBox->currentText().trimmed(), buffer);
			settings.endGroup();
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
	}
}


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
