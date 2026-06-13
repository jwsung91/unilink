# Callback Data Lifetime

`MessageContext::data()` returns a callback-scoped view. The view is valid only
while the current callback is running.

Use an ownership-copy helper before sending data to another thread, queue,
coroutine, or longer-lived object.

```cpp
client->on_data([](const unilink::MessageContext& ctx) {
    auto owned = ctx.data_as_string();
    post_to_worker([owned = std::move(owned)] {
        process(owned);
    });
});
```

Do not store `std::string_view` or spans returned from callback context objects.

```cpp
client->on_data([](const unilink::MessageContext& ctx) {
    auto view = ctx.data();
    post_to_worker([view] {
        process(view);  // dangling risk
    });
});
```

For binary payloads, use `data_as_vector()` before the callback returns.
