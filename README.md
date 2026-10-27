# Auto Battle Sandbox

Auto Battle Sandbox is an Unreal Engine 5.8 C++ project for experimenting with data-driven, ability-based automatic
combat. It includes a simplified and adapted modular ability system based on another project I worked on.

## Included systems

- Round- and turn-based auto-battle flow with configurable attack order, target priorities, and battle rules.
- Units, teams, battlefield zones, drag-and-drop unit placement, selection, and battlefield reset controls.
- Gameplay Ability System integration with offensive and defensive attributes, damage execution, Gameplay Effects, and
  attack/death animation hooks.
- Ability sequence scheduling for ordered and nested ability reactions.
- Global gameplay events that can trigger unit abilities at battle start, during attacks, and around unit death.
- Data-driven automatic and manual ability targeting with reusable criteria and strategies, including team, self,
  adjacent-unit, random, and weakest-unit targeting.
- Reusable ability tasks for target selection, effect application, animation-aware effects, and global event
  broadcasting.
- MVVM-based UI/view models and Blueprint helper libraries.
- C++ automation tests for the ability sequence, event, targeting, and unit-ability systems.

## Requirements

- Unreal Engine 5.8
- Gameplay Abilities, Modular Ability System, Common UI, Model View ViewModel, and Niagara plugins
