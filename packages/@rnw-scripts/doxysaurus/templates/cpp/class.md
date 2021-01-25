---
id: {{docId}}
title: {{typeName}}
---

Defined in `{{fileName}}`  
Namespace: **`{{namespace}}`**  
{{#namespaceAlias}}
Namespace alias: **`{{.}}`**
{{/namespaceAlias}}

## Definition

```cpp
{{&prototype}}
```
{{brief}}

{{&summary}}

{{#sections}}

### {{title}}

Name | Description
-----|------------
{{#memberOverloads}}[**`{{&name}}`**]({{anchor}}) | {{&summary}}
{{/memberOverloads}}
{{/sections}}

{{#details}}
### Notes

{{&details}}
{{/details}}

{{#sections}}
{{#memberOverloads}}
---
## `{{&name}}`

{{#members}}

```cpp
{{&prototype}}
```

{{details}}
{{/members}}

{{/memberOverloads}}
{{/sections}}