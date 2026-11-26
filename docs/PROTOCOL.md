# LearnSmart Command Protocol (Member A part)

Start the engine with `LearnSmart.exe --engine`. Python sends **one command per line** and reads the reply.

- Fields are separated by `|`. Commas and pipes are removed from names / emails automatically.
- Every reply starts with `OK|...` or `ERROR|message`.
- List replies look like: `OK|<count>` then one `ROW|...` line per item, then `END`.
- Every command that changes data saves the CSV files straight away.
- Dates are `YYYY-MM-DD`. Student IDs are positive whole numbers.

| Command | Reply |
|---|---|
| `PING` | `OK|pong` |
| `ADD_STUDENT|id|name|email` | `OK|Student registered` |
| `GET_STUDENT|id` | `OK|id|name|email|attemptCount|pendingTasks|undoSize|averagePercent` (average has one decimal, for example `45.0`) |
| `REMOVE_STUDENT|id` | `OK|Student removed` |
| `ADD_ATTEMPT|studentId|topicId|score|totalMarks|date` | `OK|attemptId` |
| `LIST_ATTEMPTS|studentId` | `OK|n`, then `ROW|attemptId|topicId|score|totalMarks|date`, then `END` |
| `DELETE_ATTEMPT|studentId|attemptId` | `OK|Attempt deleted` |
| `ADD_TASK|studentId|topicId|priority` | `OK|taskId` (priority 0-100) |
| `LIST_TASKS|studentId` | `OK|n`, then `ROW|taskId|topicId|status|priority` (front first), then `END` |
| `COMPLETE_TASK|studentId` | `OK|taskId|topicId` (completes the FRONT task) |
| `UNDO|studentId` | `OK|Undone: ...` |
| `LIST_STUDENTS` | `OK|n`, then `ROW|id|name|email|attemptCount|pendingTasks`, then `END` |
| `SEARCH_TOPIC|studentId|topicId` | `OK|n`, then the same `ROW` lines as `LIST_ATTEMPTS` (only that topic), then `END` |
| `HASH_INDEX|id` | `OK|bucketIndex|bucketCount` (works for IDs that are not registered yet) |
| `INSPECT_HASH` | `OK|bucketCount|studentCount|collisions`, then `ROW|bucket|position|id|name` (position 0 is the chain head), then `END` |
| `INSPECT_STACK|studentId` | `OK|size|capacity`, then `ROW|position|type|taskId|topicId` (position 0 is the TOP, type is `AddTask` or `CompleteTask`), then `END` |
| `INSPECT_QUEUE|studentId` | `OK|front|rear|count|capacity`, then one `ROW|slot|used|taskId|topicId|priority` per array slot (`used` is 1 or 0), then `END` |
| `SAVE` | `OK|saved` |
| `EXIT` | `OK|bye` (saves, then the program ends) |

Member B will add their own commands (quiz scoring, recommendations, graph, heap...) to this same file.

## Example

```
ADD_ATTEMPT|1001|2|5|10|2026-11-01   ->  OK|11
ADD_TASK|1001|7|80                   ->  OK|4
COMPLETE_TASK|1001                   ->  OK|1|7
UNDO|1001                            ->  OK|Undone: Completed task #1 (topic 7)
UNKNOWN|1                            ->  ERROR|Unknown command: UNKNOWN
```
