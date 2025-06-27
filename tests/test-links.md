## Link test
My search engine is [Google](https://www.google.com "The well-known search engine").

My search engine is [Google](https://www.google.com).

My search engine is **[Google](https://www.google.com)**.

My search engine are **[Google](https://www.google.com)** and _[DuckDuckGo](https://duckduckgo.com)_.

Testing code section `[Google](https://www.google.com)` should not be emphasized.

I love supporting the ___[EFF](https://eff.org)___.

This is the *[Markdown Guide](https://www.markdownguide.org)*.

See the section on [`code`](#code).

    No links will be transformed in code blocks.
    [Google](https://www.google.com) should not be transformed in code blocks.
    [DuckDuckGo][1].
    <https://www.example.com>

Testing for reference-style link [Google][1].

Another reference-style link [DuckDuckGo] [duckduck-go] without title.

This has no reference, should not be transformed: [empty ref][not-a-reference].

Reference style with two space is not valid [Google]  [1].

Reference label is case-insensitive: [DuckDuckGo][DUCKDUCK-GO].

Testing with invalid label [Invalid][invalid-標籤] or [Invalid][label，].

Multiple links test [Google](https://www.google.com "search") and [DuckDuckGo](https://duckduckgo.com).

Multiple reference links test [Google][1] and [DuckDuckGo][duckduck-go].

Reference style within code `[Google][1]` should not be transformed,
but code within reference [`Google`][1] should be transformed.

[1]: https://www.google.com "The well-known search engine"
[duckduck-go]: https://duckduckgo.com

Testing URL and email address <https://www.markdownguide.org> with <fake@example.com>

Combine [markdown](https://www.markdownguide.org) with email **<fake@example.com>**
