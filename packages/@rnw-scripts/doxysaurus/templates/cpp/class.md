---
id: {{docId}}
title: {{typeName}}
---

Defined in `file name`  
Namespace: **`{{namespace}}`**  
{{#namespaceAlias}}
Namespace alias: **`{{.}}`**
{{/namespaceAlias}}

## Summary

```cpp
{{prototype}}
```

{{brief}}

{{#sections}}

### {{title}}

Name | Description
-----|------------
{{#memberOverloads}}[**`{{&name}}`**]({{anchor}})| description
{{/memberOverloads}}
{{/sections}}

{{details}}

{{#memberOverloads}}
## `{{&name}}`

Hello!
{{/memberOverloads}}