# Copilot customization coverage

This file records how the previous Copilot customization markdown structure maps to the current split between repository instructions, skills, and the custom C++ agent.

## Original markdown structure

| file name | paragraph | sub-paragraph |
| --- | --- | --- |
| `.github/copilot-instructions.md` | MSAPI Project Overview | Key characteristics |
| `.github/copilot-instructions.md` | Architecture | Core Components (Concrete File References) |
| `.github/copilot-instructions.md` | Architecture | Interaction Overview |
| `.github/copilot-instructions.md` | Architecture | Extension Points |
| `.github/copilot-instructions.md` | Data Models & Ownership | Framing & Headers |
| `.github/copilot-instructions.md` | Data Models & Ownership | Standard Protocol |
| `.github/copilot-instructions.md` | Data Models & Ownership | Object Protocol |
| `.github/copilot-instructions.md` | Data Models & Ownership | Tabular Data |
| `.github/copilot-instructions.md` | Data Models & Ownership | Parameters |
| `.github/copilot-instructions.md` | Data Models & Ownership | Memory Strategy |
| `.github/copilot-instructions.md` | Coding Standards | C++ Style |
| `.github/copilot-instructions.md` | Coding Standards | File Headers |
| `.github/copilot-instructions.md` | Coding Standards | Enum Guidance |
| `.github/copilot-instructions.md` | Building & Testing | Environment Setup |
| `.github/copilot-instructions.md` | Building & Testing | Helper Scripts (Preferred) |
| `.github/copilot-instructions.md` | Building & Testing | Direct CMake |
| `.github/copilot-instructions.md` | Building & Testing | Testing Structure |
| `.github/copilot-instructions.md` | Building & Testing | CI Workflows |
| `.github/copilot-instructions.md` | Common Patterns | Server Implementation |
| `.github/copilot-instructions.md` | Common Patterns | Application State Management |
| `.github/copilot-instructions.md` | Common Patterns | Logging |
| `.github/copilot-instructions.md` | Common Patterns | Testing (C++) |
| `.github/copilot-instructions.md` | Frontend Workflow (Manager App) | Current State |
| `.github/copilot-instructions.md` | Frontend Workflow (Manager App) | JS Core |
| `.github/copilot-instructions.md` | Frontend Workflow (Manager App) | Default views |
| `.github/copilot-instructions.md` | Frontend Workflow (Manager App) | JS Style & Conventions |
| `.github/copilot-instructions.md` | Frontend Workflow (Manager App) | Frontend Testing (Node-Based) |
| `.github/copilot-instructions.md` | Frontend Workflow (Manager App) | Security Considerations |
| `.github/copilot-instructions.md` | Performance Recommendations | Server Loop |
| `.github/copilot-instructions.md` | Performance Recommendations | Protocol Encode/Decode |
| `.github/copilot-instructions.md` | Performance Recommendations | Logging |
| `.github/copilot-instructions.md` | Important Directories | — |
| `.github/copilot-instructions.md` | Requirements | — |
| `.github/copilot-instructions.md` | Contributor Checklist | — |
| `.github/agents/expert-cpp-software-engineer.md` | Role | — |
| `.github/agents/expert-cpp-software-engineer.md` | Context | — |
| `.github/agents/expert-cpp-software-engineer.md` | Outputs | — |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Standards and Context |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Modern C++ and Ownership |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Error Handling and Contracts |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Concurrency and Performance |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Architecture and Domain Boundaries |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Testing |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Legacy Code and Incremental Refactoring |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Build, Tooling, API/ABI, Portability |
| `.github/agents/expert-cpp-software-engineer.md` | When proposing code or refactors | — |

## Current markdown structure

| file name | paragraph | sub-paragraph |
| --- | --- | --- |
| `.github/copilot-instructions.md` | Project baseline | — |
| `.github/copilot-instructions.md` | Repository-wide expectations | — |
| `.github/copilot-instructions.md` | Validation commands | — |
| `.github/copilot-instructions.md` | Use skills for task-specific guidance | — |
| `.github/skills/msapi-architecture/SKILL.md` | MSAPI Project Overview | Key characteristics |
| `.github/skills/msapi-architecture/SKILL.md` | Architecture | Core Components (Concrete File References) |
| `.github/skills/msapi-architecture/SKILL.md` | Architecture | Interaction Overview |
| `.github/skills/msapi-architecture/SKILL.md` | Architecture | Extension Points |
| `.github/skills/msapi-architecture/SKILL.md` | Data Models & Ownership | Framing & Headers |
| `.github/skills/msapi-architecture/SKILL.md` | Data Models & Ownership | Standard Protocol |
| `.github/skills/msapi-architecture/SKILL.md` | Data Models & Ownership | Object Protocol |
| `.github/skills/msapi-architecture/SKILL.md` | Data Models & Ownership | Tabular Data |
| `.github/skills/msapi-architecture/SKILL.md` | Data Models & Ownership | Parameters |
| `.github/skills/msapi-architecture/SKILL.md` | Data Models & Ownership | Memory Strategy |
| `.github/skills/msapi-architecture/SKILL.md` | Important Directories | — |
| `.github/skills/msapi-architecture/SKILL.md` | Requirements | — |
| `.github/skills/msapi-server-and-protocols/SKILL.md` | Server framework | — |
| `.github/skills/msapi-server-and-protocols/SKILL.md` | Application lifecycle | — |
| `.github/skills/msapi-server-and-protocols/SKILL.md` | Authorization | — |
| `.github/skills/msapi-server-and-protocols/SKILL.md` | Protocol rules | — |
| `.github/skills/msapi-server-and-protocols/SKILL.md` | Common Patterns | Server Implementation |
| `.github/skills/msapi-server-and-protocols/SKILL.md` | Common Patterns | Application State Management |
| `.github/skills/msapi-server-and-protocols/SKILL.md` | Practical guidance | — |
| `.github/skills/msapi-cpp-conventions/SKILL.md` | Coding Standards | C++ Style |
| `.github/skills/msapi-cpp-conventions/SKILL.md` | Coding Standards | File Headers |
| `.github/skills/msapi-cpp-conventions/SKILL.md` | Coding Standards | Enum Guidance |
| `.github/skills/msapi-cpp-conventions/SKILL.md` | Common Patterns | Logging |
| `.github/skills/msapi-build-and-testing/SKILL.md` | Building & Testing | Environment Setup |
| `.github/skills/msapi-build-and-testing/SKILL.md` | Building & Testing | Helper Scripts (Preferred) |
| `.github/skills/msapi-build-and-testing/SKILL.md` | Building & Testing | Direct CMake |
| `.github/skills/msapi-build-and-testing/SKILL.md` | Building & Testing | Testing Structure |
| `.github/skills/msapi-build-and-testing/SKILL.md` | Building & Testing | CI Workflows |
| `.github/skills/msapi-build-and-testing/SKILL.md` | Common Patterns | Testing (C++) |
| `.github/skills/msapi-build-and-testing/SKILL.md` | Contributor Checklist | — |
| `.github/skills/msapi-manager-frontend/SKILL.md` | Frontend Workflow (Manager App) | Current State |
| `.github/skills/msapi-manager-frontend/SKILL.md` | Frontend Workflow (Manager App) | JS Core |
| `.github/skills/msapi-manager-frontend/SKILL.md` | Frontend Workflow (Manager App) | Default views |
| `.github/skills/msapi-manager-frontend/SKILL.md` | Frontend Workflow (Manager App) | JS Style & Conventions |
| `.github/skills/msapi-manager-frontend/SKILL.md` | Frontend Workflow (Manager App) | Frontend Testing (Node-Based) |
| `.github/skills/msapi-manager-frontend/SKILL.md` | Frontend Workflow (Manager App) | Security Considerations |
| `.github/skills/msapi-performance-and-reliability/SKILL.md` | Performance Recommendations | Server Loop |
| `.github/skills/msapi-performance-and-reliability/SKILL.md` | Performance Recommendations | Protocol Encode/Decode |
| `.github/skills/msapi-performance-and-reliability/SKILL.md` | Performance Recommendations | Logging |
| `.github/agents/expert-cpp-software-engineer.md` | Role | — |
| `.github/agents/expert-cpp-software-engineer.md` | Context | — |
| `.github/agents/expert-cpp-software-engineer.md` | Outputs | — |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Standards and Context |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Modern C++ and Ownership |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Error Handling and Contracts |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Concurrency and Performance |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Architecture and Domain Boundaries |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Testing |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Legacy Code and Incremental Refactoring |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Build, Tooling, API/ABI, Portability |
| `.github/agents/expert-cpp-software-engineer.md` | How to use repository context | — |
| `.github/agents/expert-cpp-software-engineer.md` | When proposing code or refactors | — |

## Coverage mapping

| original file name | original paragraph | original sub-paragraph | current file name | current paragraph | current sub-paragraph |
| --- | --- | --- | --- | --- | --- |
| `.github/copilot-instructions.md` | MSAPI Project Overview | Key characteristics | `.github/skills/msapi-architecture/SKILL.md` | MSAPI Project Overview | Key characteristics |
| `.github/copilot-instructions.md` | Architecture | Core Components (Concrete File References) | `.github/skills/msapi-architecture/SKILL.md` | Architecture | Core Components (Concrete File References) |
| `.github/copilot-instructions.md` | Architecture | Interaction Overview | `.github/skills/msapi-architecture/SKILL.md` | Architecture | Interaction Overview |
| `.github/copilot-instructions.md` | Architecture | Extension Points | `.github/skills/msapi-architecture/SKILL.md` | Architecture | Extension Points |
| `.github/copilot-instructions.md` | Data Models & Ownership | Framing & Headers | `.github/skills/msapi-architecture/SKILL.md` | Data Models & Ownership | Framing & Headers |
| `.github/copilot-instructions.md` | Data Models & Ownership | Standard Protocol | `.github/skills/msapi-architecture/SKILL.md` | Data Models & Ownership | Standard Protocol |
| `.github/copilot-instructions.md` | Data Models & Ownership | Object Protocol | `.github/skills/msapi-architecture/SKILL.md` | Data Models & Ownership | Object Protocol |
| `.github/copilot-instructions.md` | Data Models & Ownership | Tabular Data | `.github/skills/msapi-architecture/SKILL.md` | Data Models & Ownership | Tabular Data |
| `.github/copilot-instructions.md` | Data Models & Ownership | Parameters | `.github/skills/msapi-architecture/SKILL.md` | Data Models & Ownership | Parameters |
| `.github/copilot-instructions.md` | Data Models & Ownership | Memory Strategy | `.github/skills/msapi-architecture/SKILL.md` | Data Models & Ownership | Memory Strategy |
| `.github/copilot-instructions.md` | Coding Standards | C++ Style | `.github/skills/msapi-cpp-conventions/SKILL.md` | Coding Standards | C++ Style |
| `.github/copilot-instructions.md` | Coding Standards | File Headers | `.github/skills/msapi-cpp-conventions/SKILL.md` | Coding Standards | File Headers |
| `.github/copilot-instructions.md` | Coding Standards | Enum Guidance | `.github/skills/msapi-cpp-conventions/SKILL.md` | Coding Standards | Enum Guidance |
| `.github/copilot-instructions.md` | Building & Testing | Environment Setup | `.github/skills/msapi-build-and-testing/SKILL.md` | Building & Testing | Environment Setup |
| `.github/copilot-instructions.md` | Building & Testing | Helper Scripts (Preferred) | `.github/skills/msapi-build-and-testing/SKILL.md` | Building & Testing | Helper Scripts (Preferred) |
| `.github/copilot-instructions.md` | Building & Testing | Direct CMake | `.github/skills/msapi-build-and-testing/SKILL.md` | Building & Testing | Direct CMake |
| `.github/copilot-instructions.md` | Building & Testing | Testing Structure | `.github/skills/msapi-build-and-testing/SKILL.md` | Building & Testing | Testing Structure |
| `.github/copilot-instructions.md` | Building & Testing | CI Workflows | `.github/skills/msapi-build-and-testing/SKILL.md` | Building & Testing | CI Workflows |
| `.github/copilot-instructions.md` | Common Patterns | Server Implementation | `.github/skills/msapi-server-and-protocols/SKILL.md` | Common Patterns | Server Implementation |
| `.github/copilot-instructions.md` | Common Patterns | Application State Management | `.github/skills/msapi-server-and-protocols/SKILL.md` | Common Patterns | Application State Management |
| `.github/copilot-instructions.md` | Common Patterns | Logging | `.github/skills/msapi-cpp-conventions/SKILL.md` | Common Patterns | Logging |
| `.github/copilot-instructions.md` | Common Patterns | Testing (C++) | `.github/skills/msapi-build-and-testing/SKILL.md` | Common Patterns | Testing (C++) |
| `.github/copilot-instructions.md` | Frontend Workflow (Manager App) | Current State | `.github/skills/msapi-manager-frontend/SKILL.md` | Frontend Workflow (Manager App) | Current State |
| `.github/copilot-instructions.md` | Frontend Workflow (Manager App) | JS Core | `.github/skills/msapi-manager-frontend/SKILL.md` | Frontend Workflow (Manager App) | JS Core |
| `.github/copilot-instructions.md` | Frontend Workflow (Manager App) | Default views | `.github/skills/msapi-manager-frontend/SKILL.md` | Frontend Workflow (Manager App) | Default views |
| `.github/copilot-instructions.md` | Frontend Workflow (Manager App) | JS Style & Conventions | `.github/skills/msapi-manager-frontend/SKILL.md` | Frontend Workflow (Manager App) | JS Style & Conventions |
| `.github/copilot-instructions.md` | Frontend Workflow (Manager App) | Frontend Testing (Node-Based) | `.github/skills/msapi-manager-frontend/SKILL.md` | Frontend Workflow (Manager App) | Frontend Testing (Node-Based) |
| `.github/copilot-instructions.md` | Frontend Workflow (Manager App) | Security Considerations | `.github/skills/msapi-manager-frontend/SKILL.md` | Frontend Workflow (Manager App) | Security Considerations |
| `.github/copilot-instructions.md` | Performance Recommendations | Server Loop | `.github/skills/msapi-performance-and-reliability/SKILL.md` | Performance Recommendations | Server Loop |
| `.github/copilot-instructions.md` | Performance Recommendations | Protocol Encode/Decode | `.github/skills/msapi-performance-and-reliability/SKILL.md` | Performance Recommendations | Protocol Encode/Decode |
| `.github/copilot-instructions.md` | Performance Recommendations | Logging | `.github/skills/msapi-performance-and-reliability/SKILL.md` | Performance Recommendations | Logging |
| `.github/copilot-instructions.md` | Important Directories | — | `.github/skills/msapi-architecture/SKILL.md` | Important Directories | — |
| `.github/copilot-instructions.md` | Requirements | — | `.github/skills/msapi-architecture/SKILL.md` | Requirements | — |
| `.github/copilot-instructions.md` | Contributor Checklist | — | `.github/skills/msapi-build-and-testing/SKILL.md` | Contributor Checklist | — |
| `.github/agents/expert-cpp-software-engineer.md` | Role | — | `.github/agents/expert-cpp-software-engineer.md` | Role | — |
| `.github/agents/expert-cpp-software-engineer.md` | Context | — | `.github/agents/expert-cpp-software-engineer.md` | Context | — |
| `.github/agents/expert-cpp-software-engineer.md` | Outputs | — | `.github/agents/expert-cpp-software-engineer.md` | Outputs | — |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Standards and Context | `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Standards and Context |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Modern C++ and Ownership | `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Modern C++ and Ownership |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Error Handling and Contracts | `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Error Handling and Contracts |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Concurrency and Performance | `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Concurrency and Performance |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Architecture and Domain Boundaries | `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Architecture and Domain Boundaries |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Testing | `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Testing |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Legacy Code and Incremental Refactoring | `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Legacy Code and Incremental Refactoring |
| `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Build, Tooling, API/ABI, Portability | `.github/agents/expert-cpp-software-engineer.md` | C++ principles | Build, Tooling, API/ABI, Portability |
| `.github/agents/expert-cpp-software-engineer.md` | When proposing code or refactors | — | `.github/agents/expert-cpp-software-engineer.md` | When proposing code or refactors | — |

## Skill layout note

GitHub project skills are stored as one directory per skill with a required `SKILL.md` file inside that directory. This layout follows the GitHub Copilot skill specification and leaves room for skill-specific resources later if the repository needs them.
