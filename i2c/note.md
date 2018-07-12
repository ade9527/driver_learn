## 迭代所有注册的设备driver:
```
i2c_register_adapter()
	klist_iter_init_node(&bus->p->klist_drivers, &i, start ? &start->p->knode_bus : NULL);
	while ((drv = next_driver(&i)) && !error)
		error = fn(drv, data);
	klist_iter_exit(&i);

```

void klist_iter_init_node(struct klist *k, struct klist_iter *i,
			  struct klist_node *n)
{
	i->i_klist = k;
	i->i_cur = NULL;
	if (n && kref_get_unless_zero(&n->n_ref))
		i->i_cur = n;
}


`i2c_new_device` 认为设备肯定存在
`i2c_new_probed_device` :对于已经识别出来的设备 才会`new`

## i2c bus 的device driver
### 使用i2c读写函数时可能会出现错误
at24.c(eeprom)中的处理是 设置一个timeout出现错误时 没有timeout就重试
```
/*
	 * Reads fail if the previous write didn't complete yet. We may
	 * loop a few times until this one succeeds, waiting at least
	 * long enough for one entire page write to work.
	 */
	timeout = jiffies + msecs_to_jiffies(write_timeout);
	do {
		read_time = jiffies;
		if (at24->use_smbus) {
			status = i2c_smbus_read_i2c_block_data_or_emulated(client, offset,
									   count, buf);
		} else {
			status = i2c_transfer(client->adapter, msg, 2);
			if (status == 2)
				status = count;
		}
		dev_dbg(&client->dev, "read %zu@%d --> %d (%ld)\n",
				count, offset, status, jiffies);

		if (status == count)
			return count;

		/* REVISIT: at HZ=100, this is sloooow */
		msleep(1);
	} while (time_before(read_time, timeout));
```

