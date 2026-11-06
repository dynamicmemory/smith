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


from abc import ABC, abstractmethod

class BaseTool(ABC):
    """ Abstract base class for all tools. """

    def __init__(self, name: str, description: str, tool_definition: dict[str, Any]):
        self.name = name or self.__class__.__name__ 
        self.description = description or self.__doc__ or ""
        self._tool_definition = tool_definition


    @property
    def tool_definition(self) -> dict[str, Any] | None:
        return self._tool_definition


    @abstractmethod
    def execute(self, context: ExecutionContext, **kwargs) -> Any:
        pass 


    def __call__(self, context: ExecutionContext, **kwargs) -> Any:
        return self.execute(context, **kwargs)


from typing import Callable

class FunctionTool(BaseTool):
    """ Wraps a ptyhon function as a basetool. """

    def __init__(self, func: Callable, name: str, description: str, tool_definition: dict[str, Any]):
        self.func = func 
        self.needs_context = "context" in inspect.signature(func).parameters 
        name = name or func.__name__
        description = description or (func.__doc__ or "").strip()
        tool_definition = tool_definition or self._generate_definition()

        super().__init__(name=name, description=description, tool_definition=tool_definition)


    def execute(self, context: ExecutionContext, **kwargs) -> Any:
        """ Execute the wrapped function. """ 
        if self.needs_context:
            result = self.func(context=context, **kwargs)
        else:
            result = self.func(**kwargs)

        return result 


    def _generate_definition(self) -> dict[str, Any]:
        """ Generate tool definition from function signature """ 
        parameters = function_to_input_schema(self.func)
        return format_tool_definition(self.name, self.description, parameters)
