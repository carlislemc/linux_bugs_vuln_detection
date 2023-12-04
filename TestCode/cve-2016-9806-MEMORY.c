static int netlink_dump(struct sock *sk)
{
	struct netlink_sock *nlk = nlk_sk(sk);
	struct netlink_callback *cb;
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh;
	int len, err = -ENOBUFS;
	int alloc_size;

	mutex_lock(nlk->cb_mutex);
	if (!nlk->cb_running) {
		err = -EINVAL;
		goto errout_skb;
	}

	cb = &nlk->cb;
	alloc_size = max_t(int, cb->min_dump_alloc, NLMSG_GOODSIZE);

	if (!netlink_rx_is_mmaped(sk) &&
	    atomic_read(&sk->sk_rmem_alloc) >= sk->sk_rcvbuf)
		goto errout_skb;
	skb = netlink_alloc_skb(sk, alloc_size, nlk->portid, GFP_KERNEL);
	if (!skb)
		goto errout_skb;
	netlink_skb_set_owner_r(skb, sk);

	len = cb->dump(skb, cb);

	if (len > 0) {
		mutex_unlock(nlk->cb_mutex);

		if (sk_filter(sk, skb))
			kfree_skb(skb);
		else
			__netlink_sendskb(sk, skb);
		return 0;
	}

	nlh = nlmsg_put_answer(skb, cb, NLMSG_DONE, sizeof(len), NLM_F_MULTI);
	if (!nlh)
		goto errout_skb;

	nl_dump_check_consistent(cb, nlh);

	memcpy(nlmsg_data(nlh), &len, sizeof(len));

	if (sk_filter(sk, skb))
		kfree_skb(skb);
	else
		__netlink_sendskb(sk, skb);

	if (cb->done)
		cb->done(cb);

	nlk->cb_running = false;
	mutex_unlock(nlk->cb_mutex);
	module_put(cb->module);
	consume_skb(cb->skb);
	return 0;

errout_skb:
	mutex_unlock(nlk->cb_mutex);
	kfree_skb(skb);
	return err;
}
