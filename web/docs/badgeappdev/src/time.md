# Time

```C
struct timeval rtc_get_time_of_day(void);
```

## Example

```C
int current_time = (int) rtc_get_time_of_day().tv_sec
```