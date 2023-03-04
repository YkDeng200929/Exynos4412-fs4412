static int s3c24xx_i2c_probe(struct platform_device *pdev)
{
	struct s3c24xx_i2c *i2c;
	struct s3c2410_platform_i2c *pdata;
	struct resource *res;
	int ret;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "no platform data\n");
		return -EINVAL;
	}

	i2c = kzalloc(sizeof(struct s3c24xx_i2c), GFP_KERNEL);
	if (!i2c) {
		dev_err(&pdev->dev, "no memory for state\n");
		return -ENOMEM;
	}

	strlcpy(i2c->adap.name, "s3c2410-i2c", sizeof(i2c->adap.name));
	i2c->adap.owner   = THIS_MODULE;
	i2c->adap.algo    = &s3c24xx_i2c_algorithm;
	i2c->adap.retries = 2;
	i2c->adap.class   = I2C_CLASS_HWMON | I2C_CLASS_SPD;
	i2c->tx_setup     = 50;

	spin_lock_init(&i2c->lock);
	init_waitqueue_head(&i2c->wait);

	/* find the clock and enable it */

	i2c->dev = &pdev->dev;
	i2c->clk = clk_get(&pdev->dev, "i2c");
	if (IS_ERR(i2c->clk)) {
		dev_err(&pdev->dev, "cannot get clock\n");
		ret = -ENOENT;
		goto err_noclk;
	}

	dev_dbg(&pdev->dev, "clock source %p\n", i2c->clk);

	clk_enable(i2c->clk);

	/* map the registers */

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "cannot find IO resource\n");
		ret = -ENOENT;
		goto err_clk;
	}

	i2c->ioarea = request_mem_region(res->start, resource_size(res),
					 pdev->name);

	if (i2c->ioarea == NULL) {
		dev_err(&pdev->dev, "cannot request IO\n");
		ret = -ENXIO;
		goto err_clk;
	}

	i2c->regs = ioremap(res->start, resource_size(res));

	if (i2c->regs == NULL) {
		dev_err(&pdev->dev, "cannot map IO\n");
		ret = -ENXIO;
		goto err_ioarea;
	}

	dev_dbg(&pdev->dev, "registers %p (%p, %p)\n",
		i2c->regs, i2c->ioarea, res);

	/* setup info block for the i2c core */

	i2c->adap.algo_data = i2c;
	i2c->adap.dev.parent = &pdev->dev;

	/* initialise the i2c controller */

	ret = s3c24xx_i2c_init(i2c);
	if (ret != 0)
		goto err_iomap;

	/* find the IRQ for this unit (note, this relies on the init call to
	 * ensure no current IRQs pending
	 */

	i2c->irq = ret = platform_get_irq(pdev, 0);
	if (ret <= 0) {
		dev_err(&pdev->dev, "cannot find IRQ\n");
		goto err_iomap;
	}

	ret = request_irq(i2c->irq, s3c24xx_i2c_irq, IRQF_DISABLED,
			  dev_name(&pdev->dev), i2c);

	if (ret != 0) {
		dev_err(&pdev->dev, "cannot claim IRQ %d\n", i2c->irq);
		goto err_iomap;
	}

	i2c->adap.nr = pdata->bus_num;

	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to add bus to i2c core\n");
		goto err_cpufreq;
	}

	platform_set_drvdata(pdev, i2c);

	dev_info(&pdev->dev, "%s: S3C I2C adapter\n", dev_name(&i2c->adap.dev));
	return 0;

 err_cpufreq:
	s3c24xx_i2c_deregister_cpufreq(i2c);

 err_irq:
	free_irq(i2c->irq, i2c);

 err_iomap:
	iounmap(i2c->regs);

 err_ioarea:
	release_resource(i2c->ioarea);
	kfree(i2c->ioarea);

 err_clk:
	clk_disable(i2c->clk);
	clk_put(i2c->clk);

 err_noclk:
	kfree(i2c);
	return ret;
}

-----------------------------------i2c-core.c------------------------------------------------------------
int i2c_add_numbered_adapter(struct i2c_adapter *adap)
{
	if (adap->nr == -1) /* -1 means dynamically assign bus id */
		return i2c_add_adapter(adap);

	return __i2c_add_numbered_adapter(adap);
}


int i2c_add_adapter(struct i2c_adapter *adapter)
{
	struct device *dev = &adapter->dev;
	int id;
	
	return i2c_register_adapter(adapter);
}

static int i2c_register_adapter(struct i2c_adapter *adap)
{	
	 of_i2c_register_devices(adap);
}


static void of_i2c_register_devices(struct i2c_adapter *adap)
{
	void *result;
	struct device_node *node;


	for_each_available_child_of_node(adap->dev.of_node, node) {
		struct i2c_board_info info = {};
		struct dev_archdata dev_ad = {};
		const __be32 *addr;
		int len;


		addr = of_get_property(node, "reg", &len);
		if (!addr || (len < sizeof(int))) {
			dev_err(&adap->dev, "of_i2c: invalid reg on %s\n",
				node->full_name);
			continue;
		}

		info.addr = be32_to_cpup(addr);
		if (info.addr > (1 << 10) - 1) {
			dev_err(&adap->dev, "of_i2c: invalid addr=%x on %s\n",
				info.addr, node->full_name);
			continue;
		}
		
		//@adap i2c控制器设备
		//@info i2c从设备信息
		result = i2c_new_device(adap, &info);
		if (result == NULL) {
			dev_err(&adap->dev, "of_i2c: Failure registering %s\n",
				node->full_name);
			of_node_put(node);
			irq_dispose_mapping(info.irq);
			continue;
		}
	}
}

struct i2c_client *
i2c_new_device(struct i2c_adapter *adap, struct i2c_board_info const *info)
{
	struct i2c_client	*client;
	int			status;

	client = kzalloc(sizeof *client, GFP_KERNEL);
	if (!client)
		return NULL;

	client->adapter = adap;

	client->dev.platform_data = info->platform_data;

	if (info->archdata)
		client->dev.archdata = *info->archdata;

	client->flags = info->flags;
	client->addr = info->addr;
	client->irq = info->irq;

	strlcpy(client->name, info->type, sizeof(client->name));

	status = device_register(&client->dev);//注册设备
	if (status)
		goto out_err;

	return client;
}

s3c2410.c的probe函数中调用注册adapter的函数接口：
i2c_add_numbered_adapter --->
				i2c_add_adapter ---> 
						i2c_register_adapter ---> 
									of_i2c_register_devices，
									
在函数of_i2c_register_devices中会遍历这个adapter对应的device node的child device node，
这些child device node对应的就是挂载i2c bus上的板级外设的硬件信息(如MPU6050)（这些板级
外设使用I2C接口跟SOC通信）然后调用of_i2c_register_device，这个函数根据每个child device 
node的信息构造i2c_board_info，并调用i2c_new_device，在i2c_new_device中会创建并注册
i2c_client，注册i2c_client的时候如果找到了对应的设备驱动程序（如MPU6050的驱动程序），
设备驱动程序的probe函数就会被调动。


问:i2c总线上的从设备是如何注册到i2c总线上的呢?
答:
	我们需要先将从设备信息（从机地址）以设备树的形式写在它对应的i2c控制器设备节点中,作为它的
	子节点存在。i2c控制器驱动加载的时候，注册i2c adapter的时候，它会扫描控制器设备的子节点。
	解析完子节点之后，将从设备的信息存放在了i2c_board_info结构体。然后根据i2c_board_info
	这个结构体创建了i2c_client(这个结构体描述是i2c总线上的从设备),并且将它注册到了i2c总线上。
	当它注册到i2c总线上之后,匹配对应的从设备驱动。匹配驱动成功之后，调用了驱动的probe函数。


1.确定设备是挂载在什么总线上?
  mpu6050 挂载在I2C总线
  
2.在总线上驱动怎么描述?
	struct i2c_driver

3.在总线上设备怎么描述?
	struct i2c_client

4.在总线上驱动怎么注册?
  i2c_add_driver

5.在总线上设备怎么注册?
  I2C控制器驱动注册i2c adapter的时候，解析了它的子节点(从设备的信息),解析完成之后注册到I2C总线

6.在总线上设备和驱动是如何匹配的呢(匹配规则)?
  <1>设备树中的compatible属性匹配
  <2>如果设备树没有匹配成功，id_table记录的名字和i2c_client中记录的名字匹配
  
  注意:
  如果没有id_table,就算你的设备树中compatible匹配成功了，也不会调用probe函数