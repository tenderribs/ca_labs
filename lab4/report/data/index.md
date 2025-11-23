Here I briefly summarize what each logfile contains, mainly so that I don't forget.

### GHB PC/CS

Two versions, turns out both have identical results. The 20b ip tag version is more true to life, so roll with this one. You are getting the same performance with a much smaller hardware budget (2.5 bytes per entry vs ~8 bytes), which is better anyway.

##### 1C_fullBW_ghb_pccs_20b_ip_tag
GHB PC/CS prefetch implementation with increased aliasing induced by restricting the bit width of the tags used to reflect the specifics of the paper. This means 1-2 bytes per table entry and using additional 4-bits for the ghb_ptr for a total of 12 bits (8 + 4). Modular arithmetic is used to guarantee correctness of roll-overs and most variables are simple `uint16_t`s.

##### 1C_fullBW_ghb_pccs_56b_ip_tag
GHB PC/CS prefetcher with 56 bit tags and up to 64 bits. Makes the coding easier, by assuming that roll-overs will never happen. But in effect this reduces aliasing, contrary to the assumptions made in the paper.