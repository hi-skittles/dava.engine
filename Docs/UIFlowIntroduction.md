# UI Flow

## Introduction

To work with new UI Flow you need only QuickEd and UIViewer. These instruments allow to create basic UI with multiple screens and some logic and data bindings. UI Flow presents hierarchy of states where each state can contain view, behavior and services.

## Quick start

1. To create simple Flow based UI your need to create empty yaml in your project.
2. Then add root control to yaml.
3. Add `UIFlowStateComponent` to this control and setup it - it marks control as UI Flow state
4. If you want you can add `UIFlowViewComponent` to this control and setup it - it marks that this state has view representation and it will be activated and appended to control on displayed screen by specified path.
5. If you want you can add `UIFlowControllerComponent` to this control and setup it - it marks that this state has controller which should work while this state is activated. Controller can be presented as cpp native code with reflection (and registered reflection permanent name) or lua script with specified functions.
6. If you want you can add `UIFlowTransitionComponent` to this control and setup it - it marks that this state has transition rules. Rule is a pair of event's name and state's path which should be activated after firing specified event.
7. Next you can add as many states as you want across steps above.

## Rules

### Life cycle rules

1. State activation will activate all parent states until root element.
2. Switching state will deactivate all states until common parent state between old and new activated states.
3. Activating state includes:
    - setup context (activating services)
    - initialize controller with context
    - load view, bind data to it and loading specified resources from controller (it step can be executed in other thread)
    - append the view to the control with specified path
    - activating controller with context and loaded view
4. Deactivating state includes previous steps in back order.

### State types rules

1. State can be one of these types:
    - `Group`, 
    - `Single`, 
    - `Multiple`.
2. `Group` states can be activated without `Single` or `Multiple` states. It mean if user try to activate Group state then first `Single` or `Multiple` child will be activated. `Group` states using for loading common controllers, resources and UI.
3. In same time could be activated only one `Single` state. It means if user activate another `Single` state previous `Single` state will be deactivated.
4. In same time could be activated some `Multiple` states. It means if user want to activate or deactivate this state his should do it manually.

### State history rules

Exist next history behaviors:
1. `None` - don't add this transition transaction to this state to history
2. `Multiple` - allows add to history few transitions transaction to this state
3. `Single` - if other transition transaction to this state already exists in history then all history entries between old transaction and new transaction will be removed and new transaction will be merged with old transaction
4. `SingleSibling` - same as `Single` but includes also siblings of this state
5. `OnlyTop` - store transition transaction to this state only if it places ot top of the history stack, overwise the transaction will be removed from history

## Components

### UIFlowStateComponent

Marks that control is UI Flow state. To setup this component (and state) user should select type of state and type of history behaviors (see `State types rules`). Also user can setup with services will be created with activation on this state, and which events will be sent to systems if it state has been activated or deactivated. To activate service you should write service alias (for searching its service in controllers) and native reflection permanent name of cpp type. After activation state specified service will be available by `context.getService('alias')`.

### UIFlowViewComponent

Component configures view representation of state. It contains path to yaml with content, control name which will be loaded from yaml and path to control-container on parent view where the loaded view will be added. Also this component configures data-binding data source by the name which was stored in `context.data` and data scope expression.

### UIFlowControllerComponent

Component attaches controller (native or lua) to the state. Native controller should be inherited from `UIFlowController`.

Lua script can contains next functions:
- `init(context)`,
- `activate(context, view)`,
- `loadResources(context, view)`,
- `process(frameDelta)`,
- `processEvent(eventName)` (must return true for avoid sending current event next),
- `deactivate(context, view)`,
- `unloadResources(context, view)`,
- `release(context)`.

### UIFlowTransitionComponent

Component configures possible transitions rules from current state to another states by specified events. Each rule contains event name, which should be processed by this rule, type of processing and processing argument depends by selected type.
Available types are:
- `Activate` (argument is path to state which should been activate),
- `Deactivate` (argument is path to state with should been deactivate),
- `ActivateBackground` (same as `Activate` but loading step will be executed in other thread),
- `DeactivateBackground` (same as `Deactivate` but unloading step will be executed in other thread),
- `SendEvent` (argument is new event which should been fire),
- `HistoryBack` (without arguments).

# Example of flow structure

```
 ____________________________________________
| Main                                       |
|--------------------------------------------|
| Type: Group                                |
| History: None                              |
|--------------------------------------------|
| Lua controller: <path>                     |
|--------------------------------------------|
| Transitions:                               |
| BACK -> HistoryBack                        |
|____________________________________________|
    |
    |    ____________________________________________
    |___| MainMenu                                   |
    |   |--------------------------------------------|
    |   | Type: Group                                |
    |   | History: None                              |
    |   |____________________________________________|
    |       |
    |       |    ____________________________________________
    |       |___| Splash                                     |
    |       |   |--------------------------------------------|
    |       |   | Type: Single                               |
    |       |   | History: OnlyTop                           |
    |       |   |--------------------------------------------|
    |       |   | View: <path>                               |
    |       |   |--------------------------------------------|
    |       |   | Transitions:                               |
    |       |   | SKIP -> Activate: ../Title                 |
    |       |   |____________________________________________|
    |       |
    |       |    ____________________________________________
    |       |___| Title                                      |
    |       |   |--------------------------------------------|
    |       |   | Type: Single                               |
    |       |   | History: Single                            |
    |       |   |--------------------------------------------|
    |       |   | View: <path>                               |
    |       |   |--------------------------------------------|
    |       |   | Lua controller: <path>                     |
    |       |   |--------------------------------------------|
    |       |   | Transitions:                               |
    |       |   | OPTIONS -> Activate: ../Options            |
    |       |   | START -> Activate: ^/Main/Game             |
    |       |   | EXIT -> Activate: ../QuitConfirm           |
    |       |   |____________________________________________|
    |       |
    |       |    ____________________________________________
    |       |___| Options                                    |
    |           |--------------------------------------------|
    |           | Type: Single                               |
    |           | History: Single                            |
    |           |--------------------------------------------|
    |           | View: <path>                               |
    |           |--------------------------------------------|
    |           | Transitions:                               |
    |           | START -> Activate: ^/Main/Game             |
    |           | EXIT -> Activate: ^/QuitConfirm            |
    |           |____________________________________________|
    |
    |    ____________________________________________
    |___| Game                                       |
    |   |--------------------------------------------|
    |   | Type: Single                               |
    |   | History: Single                            |
    |   |--------------------------------------------|
    |   | Lua controller: <path>                     |
    |   |--------------------------------------------|
    |   | Transitions:                               |
    |   | PAUSE -> Activate: GameMenu                |
    |   | RESUME -> Deactivate: GameMenu             |
    |   | EXIT -> Activate: ^/QuitConfirm            |
    |   |____________________________________________|
    |       |
    |       |    ____________________________________________
    |       |___| GameMenu                                   |
    |           |--------------------------------------------|
    |           | Type: Multiple                             |
    |           | History: Single                            |
    |           |--------------------------------------------|
    |           | View: <path>                               |
    |           | Container: <path>                          |
    |           |____________________________________________|
    |
    |    ____________________________________________
    |___| QuitConfirm                                |
        |--------------------------------------------|
        | Type: Multiple                             |
        | History: OnlyTop                           |
        |--------------------------------------------|
        | View: <path>                               |
        | Container: <path>                          |
        |--------------------------------------------|
        | Transitions:                               |
        | YES -> SendEvent: QUIT                     |
        | NO -> SendEvent: BACK                      |
        |____________________________________________|
```