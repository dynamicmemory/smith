"""
Prototyping before building in C++
"""

from typing import Literal, Union, Any, Optional
from dataclasses import dataclass, field 
import uuid 
from datetime import datetime


@dataclass
class Message:
    """ A text message in the conversation """
    role: Literal["system", "user", "assistant"]
    content: str
    type: Literal["message"] = "message"


@dataclass
class ToolCall:
    """ Models request to execute a tool """
    tool_call_id: str 
    name: str 
    arguments: dict 
    type: Literal["tool_call"] = "tool_call"


@dataclass
class ToolResult:
    """ Result from tool execution """
    toool_call_id: str
    name: str 
    status: Literal["success", "error"]
    content: list
    type: Literal["tool_result"] = "tool_result"


ContentItem = Union[Message, ToolCall, ToolResult]


@dataclass 
class Event:
    """ A recorded occurrence during agent execution """
    execution_id: str 
    author: str 
    id: str = field(default_factory=lambda: str(uuid.uuid4()))
    timestamp: float = field(default_factory=lambda: datetime.now().timestamp())
    content: list = field(default_factory=list)


@dataclass 
class ExecutionContext:
    """ Central storage for all execution state """
    execution_id: str = field(default_factory=lambda: str(uuid.uuid4()))
    events: list[Event] = field(default_factory=list)
    current_step: int = 0
    state: dict[str, Any] = field(default_factory=dict)
    final_result: Any = None


    def add_event(self, event: Event) -> None:
        """ Append an event to the execution history """
        self.events.append(event)

    
    def increment_step(self) -> None:
        """ Move to the next execution step. """
        self.current_step += 1
